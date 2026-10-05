#!/usr/bin/env python3
"""Web gateway for the Asynchronous File Processing Server.

Browsers cannot open raw TCP sockets, so this small bridge serves the web UI
and translates HTTP/JSON calls into the server's text protocol.
Standard library only.

  python3 gateway.py                      # UI on :8080, C server on 127.0.0.1:9090
  python3 gateway.py --web-port 8000 --storage /path/to/storage
"""
import argparse, json, re, socket, threading, time
from concurrent.futures import ThreadPoolExecutor
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument("--host", default="127.0.0.1", help="C server host")
ap.add_argument("--port", type=int, default=9090, help="C server port")
ap.add_argument("--web-port", type=int, default=8080)
ap.add_argument("--storage", default="storage", help="server storage dir (for the file list)")
ARGS = ap.parse_args()

HERE = Path(__file__).resolve().parent
NAME_RE = re.compile(r"^[A-Za-z0-9._-]{1,255}$")
KV_RE = re.compile(r"(\w+)=(\S+)")


class Session:
    """One TCP connection speaking the server's line protocol."""

    def __init__(self):
        self.sock = socket.create_connection((ARGS.host, ARGS.port), timeout=10)
        self.f = self.sock.makefile("rb")
        # The C server sends a multi-line banner and HELP text on connect.
        # Drain it through the QUIT help entry before sending a command.
        for _ in range(40):
            line = self.f.readline()
            if not line:
                self.sock.close()
                raise ConnectionError("file server closed during welcome banner")
            if b"QUIT" in line.upper():
                break
        else:
            self.sock.close()
            raise ConnectionError("unexpected file-server welcome text")

    def command(self, line):
        t0 = time.perf_counter()
        self.sock.sendall(line.encode() + b"\n")
        head = self.f.readline().decode(errors="replace").rstrip("\n")
        if not head:
            raise ConnectionError("server closed the connection")
        out = {"raw": head, "ok": not head.startswith("ERROR")}
        out.update({k: v for k, v in KV_RE.findall(head)})
        if head.startswith("OK READ"):
            n = int(out.get("bytes", 0))
            out["data"] = self.f.read(n).decode(errors="replace")
            self.f.read(1)  # trailing newline
        out["rtt_ms"] = round((time.perf_counter() - t0) * 1000, 3)
        return out

    def close(self):
        try:
            self.sock.sendall(b"QUIT\n"); self.f.readline()
        except OSError:
            pass
        self.sock.close()


def one_shot(line):
    s = Session()
    try:
        return s.command(line)
    finally:
        s.close()


def load_test(clients, requests):
    def worker(i):
        t0 = time.perf_counter(); lat = []; errs = 0
        s = Session()
        try:
            for j in range(requests):
                name = f"load_ui_{i}_{j}.txt"
                for line in (f"WRITE {name} request-{i}-{j}", f"READ {name}"):
                    r = s.command(line)
                    if r["ok"]: lat.append(int(r.get("latency_us", 0)))
                    else: errs += 1
        finally:
            s.close()
        return time.perf_counter() - t0, lat, errs

    t0 = time.perf_counter()
    with ThreadPoolExecutor(max_workers=clients) as pool:
        res = list(pool.map(worker, range(clients)))
    wall = time.perf_counter() - t0
    lat = [x for r in res for x in r[1]]
    ops = clients * requests * 2
    return {
        "clients": clients, "requests": requests, "operations": ops,
        "wall_s": round(wall, 4), "throughput": round(ops / wall, 2),
        "avg_client_s": round(sum(r[0] for r in res) / len(res), 4),
        "avg_server_latency_us": round(sum(lat) / len(lat), 2) if lat else 0,
        "errors": sum(r[2] for r in res),
    }


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def reply(self, obj, code=200):
        body = json.dumps(obj).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path in ("/", "/index.html"):
            body = (HERE / "index.html").read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers(); self.wfile.write(body)
        elif self.path == "/api/ping":
            try:
                Session().close(); self.reply({"up": True, "target": f"{ARGS.host}:{ARGS.port}"})
            except OSError as e:
                self.reply({"up": False, "target": f"{ARGS.host}:{ARGS.port}", "error": str(e)})
        elif self.path == "/api/stats":
            self.safe(lambda: one_shot("STATS"))
        elif self.path == "/api/files":
            d = Path(ARGS.storage)
            files = []
            if d.is_dir():
                for p in sorted(d.iterdir()):
                    if p.is_file() and not p.name.startswith("."):
                        st = p.stat()
                        files.append({"name": p.name, "size": st.st_size, "mtime": st.st_mtime})
            self.reply({"files": files, "dir_found": d.is_dir()})
        else:
            self.reply({"error": "not found"}, 404)

    def do_POST(self):
        try:
            n = int(self.headers.get("Content-Length", 0))
            body = json.loads(self.rfile.read(n) or b"{}")
        except (ValueError, json.JSONDecodeError):
            return self.reply({"error": "invalid JSON"}, 400)

        if self.path == "/api/write":
            name, data = str(body.get("file", "")), str(body.get("data", ""))
            if not NAME_RE.match(name) or ".." in name:
                return self.reply({"error": "Filename may use letters, digits, . _ - only (no '..')."}, 400)
            if not data or "\n" in data or "\r" in data:
                return self.reply({"error": "Data must be one non-empty line (the protocol is line based)."}, 400)
            if len(data) > 3500:
                return self.reply({"error": "Data too long for one request line (max ~3500 characters)."}, 400)
            self.safe(lambda: one_shot(f"WRITE {name} {data}"))
        elif self.path == "/api/read":
            name = str(body.get("file", ""))
            if not NAME_RE.match(name) or ".." in name:
                return self.reply({"error": "Invalid filename."}, 400)
            self.safe(lambda: one_shot(f"READ {name}"))
        elif self.path == "/api/append":
            name, data = str(body.get("file", "")), str(body.get("data", ""))
            if not NAME_RE.match(name) or ".." in name:
                return self.reply({"error": "Invalid filename."}, 400)
            if not data or "\n" in data or "\r" in data:
                return self.reply({"error": "Data must be one non-empty line."}, 400)
            if len(data) > 3500:
                return self.reply({"error": "Data too long (maximum 3500 characters)."}, 400)
            self.safe(lambda: one_shot(f"APPEND {name} {data}"))
        elif self.path == "/api/delete":
            name = str(body.get("file", ""))
            if not NAME_RE.match(name) or ".." in name:
                return self.reply({"error": "Invalid filename."}, 400)
            self.safe(lambda: one_shot(f"DELETE {name}"))
        elif self.path == "/api/rename":
            name, new_name = str(body.get("file", "")), str(body.get("new_file", ""))
            if not NAME_RE.match(name) or ".." in name or not NAME_RE.match(new_name) or ".." in new_name:
                return self.reply({"error": "Invalid filename. Use letters, digits, dot, underscore, or hyphen."}, 400)
            if name == new_name:
                return self.reply({"error": "New filename must be different."}, 400)
            self.safe(lambda: one_shot(f"RENAME {name} {new_name}"))
        elif self.path == "/api/load":
            c = max(1, min(int(body.get("clients", 10)), 200))
            r = max(1, min(int(body.get("requests", 10)), 200))
            self.safe(lambda: load_test(c, r))
        else:
            self.reply({"error": "not found"}, 404)

    def safe(self, fn):
        try:
            self.reply(fn())
        except (OSError, ConnectionError) as e:
            self.reply({"error": f"Cannot reach the file server at {ARGS.host}:{ARGS.port} ({e}). Is ./server running?"}, 502)


if __name__ == "__main__":
    srv = ThreadingHTTPServer(("127.0.0.1", ARGS.web_port), Handler)
    print(f"Web UI:      http://127.0.0.1:{ARGS.web_port}")
    print(f"File server: {ARGS.host}:{ARGS.port}   storage dir: {ARGS.storage}")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        pass
