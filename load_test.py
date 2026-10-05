#!/usr/bin/env python3
import socket
import sys
import time
from concurrent.futures import ThreadPoolExecutor

HOST = "127.0.0.1"
PORT = 9090

def run_client(index, requests):
    start = time.perf_counter()
    with socket.create_connection((HOST, PORT), timeout=10) as s:
        s.recv(4096)
        for i in range(requests):
            filename = f"load_{index}_{i}.txt"
            message = f"WRITE {filename} request-{index}-{i}\n"
            s.sendall(message.encode())
            s.recv(4096)

            message = f"READ {filename}\n"
            s.sendall(message.encode())
            s.recv(8192)
        s.sendall(b"QUIT\n")
        s.recv(4096)
    return time.perf_counter() - start

def main():
    clients = int(sys.argv[1]) if len(sys.argv) > 1 else 10
    requests = int(sys.argv[2]) if len(sys.argv) > 2 else 10

    overall = time.perf_counter()
    with ThreadPoolExecutor(max_workers=clients) as pool:
        times = list(pool.map(lambda i: run_client(i, requests), range(clients)))
    elapsed = time.perf_counter() - overall

    total_ops = clients * requests * 2
    print(f"Clients: {clients}")
    print(f"Requests per client: {requests}")
    print(f"File operations: {total_ops}")
    print(f"Wall time: {elapsed:.4f} s")
    print(f"Throughput: {total_ops / elapsed:.2f} operations/s")
    print(f"Average client time: {sum(times) / len(times):.4f} s")

if __name__ == "__main__":
    main()
