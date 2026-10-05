#define _GNU_SOURCE
#include "server.h"

#include <liburing.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

static request_queue_t request_q;
static completion_queue_t completion_q;
static atomic_ullong request_counter = 0;
static atomic_ullong completed_counter = 0;
static atomic_ullong failed_counter = 0;
static atomic_ullong total_latency_us = 0;
static volatile sig_atomic_t running = 1;
static int listen_fd = -1;

static void stop_server(int sig) {
    (void)sig;
    running = 0;
    if (listen_fd >= 0) close(listen_fd);
    request_queue_stop(&request_q);
    completion_queue_stop(&completion_q);
}

static void print_error_completion(request_t *req, const char *msg) {
    completion_t *c = calloc(1, sizeof(*c));
    if (!c) return;
    c->client_fd = req->client_fd;
    c->op = req->op;
    strncpy(c->filename, req->filename, MAX_FILENAME);
    c->data = strdup(msg);
    c->data_len = c->data ? strlen(c->data) : 0;
    c->result = -1;
    c->request_id = req->id;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    c->latency_us = elapsed_us(&req->start_time, &now);
    completion_queue_push(&completion_q, c);
}

static int parse_request(char *line, int client_fd, request_t **out) {
    char command[16] = {0};
    char filename[MAX_FILENAME + 1] = {0};

    if (sscanf(line, "%15s %255s", command, filename) < 1)
        return -1;

    request_t *req = calloc(1, sizeof(*req));
    if (!req) return -1;

    req->client_fd = client_fd;
    req->id = atomic_fetch_add(&request_counter, 1) + 1;
    clock_gettime(CLOCK_MONOTONIC, &req->start_time);

    if (strcmp(command, "READ") == 0) {
        if (sscanf(line, "%15s %255s", command, filename) != 2 || !safe_filename(filename)) {
            free(req);
            return -1;
        }
        req->op = OP_READ;
        strncpy(req->filename, filename, MAX_FILENAME);
    } else if (strcmp(command, "WRITE") == 0) {
        char *p = strchr(line, ' ');
        if (!p) { free(req); return -1; }
        while (*p == ' ') p++;
        char *space = strchr(p, ' ');
        if (!space) { free(req); return -1; }
        *space = '\0';
        if (!safe_filename(p)) { free(req); return -1; }

        req->op = OP_WRITE;
        strncpy(req->filename, p, MAX_FILENAME);
        char *data = space + 1;
        size_t len = strlen(data);
        while (len > 0 && (data[len - 1] == '\n' || data[len - 1] == '\r'))
            data[--len] = '\0';

        if (len > MAX_DATA) { free(req); return -1; }
        req->data = malloc(len + 1);
        if (!req->data) { free(req); return -1; }
        memcpy(req->data, data, len);
        req->data[len] = '\0';
        req->data_len = len;
    } else {
        free(req);
        return -1;
    }

    *out = req;
    return 0;
}

static void *client_handler(void *arg) {
    int fd = *(int *)arg;
    free(arg);

    char buffer[MAX_LINE];
    size_t used = 0;

    const char *welcome =
        "Asynchronous File Processing Server\n"
        "Commands: READ <file>, WRITE <file> <data>, STATS, QUIT\n";
    send(fd, welcome, strlen(welcome), MSG_NOSIGNAL);

    while (running) {
        ssize_t n = recv(fd, buffer + used, sizeof(buffer) - used - 1, 0);
        if (n <= 0) break;
        used += (size_t)n;
        buffer[used] = '\0';

        char *line_start = buffer;
        char *newline;
        while ((newline = strchr(line_start, '\n')) != NULL) {
            *newline = '\0';

            if (strcmp(line_start, "QUIT") == 0) {
                send(fd, "BYE\n", 4, MSG_NOSIGNAL);
                close(fd);
                return NULL;
            }

            if (strcmp(line_start, "STATS") == 0) {
                unsigned long long completed = atomic_load(&completed_counter);
                unsigned long long failed = atomic_load(&failed_counter);
                unsigned long long total = atomic_load(&total_latency_us);
                double avg = completed ? (double)total / completed : 0.0;
                char response[256];
                int len = snprintf(response, sizeof(response),
                    "STATS completed=%llu failed=%llu avg_latency_us=%.2f\n",
                    completed, failed, avg);
                send(fd, response, (size_t)len, MSG_NOSIGNAL);
            } else {
                request_t *req = NULL;
                if (parse_request(line_start, fd, &req) != 0) {
                    const char *err = "ERROR Invalid request. Use READ <file> or WRITE <file> <data>\n";
                    send(fd, err, strlen(err), MSG_NOSIGNAL);
                } else {
                    request_queue_push(&request_q, req);
                }
            }

            line_start = newline + 1;
        }

        size_t remaining = used - (size_t)(line_start - buffer);
        memmove(buffer, line_start, remaining);
        used = remaining;

        if (used == sizeof(buffer) - 1) {
            const char *err = "ERROR Request too long\n";
            send(fd, err, strlen(err), MSG_NOSIGNAL);
            used = 0;
        }
    }

    close(fd);
    return NULL;
}

static void *io_submitter(void *arg) {
    (void)arg;
    struct io_uring ring;

    if (io_uring_queue_init(MAX_INFLIGHT, &ring, 0) < 0) {
        perror("io_uring_queue_init");
        return NULL;
    }

    unsigned int inflight = 0;

    while (running || inflight > 0) {
        while (inflight < MAX_INFLIGHT) {
            request_t *req = request_queue_pop(&request_q);
            if (!req) break;

            char path[512];
            snprintf(path, sizeof(path), "%s/%s", STORAGE_DIR, req->filename);

            int fd;
            if (req->op == OP_READ)
                fd = open(path, O_RDONLY);
            else
                fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if (fd < 0) {
                char msg[256];
                snprintf(msg, sizeof(msg), "ERROR file open failed: %s\n", strerror(errno));
                print_error_completion(req, msg);
                free(req->data);
                free(req);
                continue;
            }

            struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
            if (!sqe) {
                close(fd);
                print_error_completion(req, "ERROR io_uring submission queue full\n");
                free(req->data);
                free(req);
                continue;
            }

            if (req->op == OP_WRITE) {
                io_uring_prep_write(sqe, fd, req->data, (unsigned)req->data_len, 0);
            } else {
                struct stat st;
                if (fstat(fd, &st) != 0) {
                    close(fd);
                    print_error_completion(req, "ERROR fstat failed\n");
                    free(req->data);
                    free(req);
                    continue;
                }

                size_t size = (size_t)st.st_size;
                if (size > MAX_DATA) {
                    close(fd);
                    print_error_completion(req, "ERROR file too large\n");
                    free(req->data);
                    free(req);
                    continue;
                }

                req->data = malloc(size + 1);
                if (!req->data) {
                    close(fd);
                    print_error_completion(req, "ERROR memory allocation failed\n");
                    free(req);
                    continue;
                }
                req->data_len = size;
                io_uring_prep_read(sqe, fd, req->data, (unsigned)size, 0);
            }

            io_uring_sqe_set_data(sqe, req);
            sqe->flags |= IOSQE_ASYNC;
            sqe->user_data = (unsigned long long)(uintptr_t)req;
            req->next = (request_t *)(intptr_t)fd;
            inflight++;

            if (io_uring_submit(&ring) < 0) {
                close(fd);
                inflight--;
                print_error_completion(req, "ERROR io_uring_submit failed\n");
                free(req->data);
                free(req);
            }
        }

        if (inflight == 0) {
            if (!running) break;
            continue;
        }

        struct io_uring_cqe *cqe = NULL;
        int ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret < 0) {
            fprintf(stderr, "io_uring_wait_cqe: %s\n", strerror(-ret));
            continue;
        }

        request_t *req = io_uring_cqe_get_data(cqe);
        int res = cqe->res;
        int fd = (int)(intptr_t)req->next;
        close(fd);

        completion_t *c = calloc(1, sizeof(*c));
        if (c) {
            c->client_fd = req->client_fd;
            c->op = req->op;
            strncpy(c->filename, req->filename, MAX_FILENAME);
            c->data = req->data;
            c->data_len = res >= 0 ? (size_t)res : 0;
            c->result = res;
            c->request_id = req->id;

            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            c->latency_us = elapsed_us(&req->start_time, &now);
            completion_queue_push(&completion_q, c);
        } else {
            free(req->data);
        }

        free(req);
        io_uring_cqe_seen(&ring, cqe);
        inflight--;
    }

    io_uring_queue_exit(&ring);
    return NULL;
}

static void *response_worker(void *arg) {
    (void)arg;

    while (running || completion_q.size > 0) {
        completion_t *c = completion_queue_pop(&completion_q);
        if (!c) {
            if (!running) break;
            continue;
        }

        if (c->result < 0) {
            char response[512];
            int len = snprintf(response, sizeof(response),
                "ERROR request=%llu file=%s code=%d latency_us=%ld\n",
                (unsigned long long)c->request_id, c->filename, c->result, c->latency_us);
            send(c->client_fd, response, (size_t)len, MSG_NOSIGNAL);
            atomic_fetch_add(&failed_counter, 1);
        } else if (c->op == OP_WRITE) {
            char response[512];
            int len = snprintf(response, sizeof(response),
                "OK WRITE request=%llu file=%s bytes=%zu latency_us=%ld\n",
                (unsigned long long)c->request_id, c->filename, c->data_len, c->latency_us);
            send(c->client_fd, response, (size_t)len, MSG_NOSIGNAL);
            free(c->data);
            atomic_fetch_add(&completed_counter, 1);
            atomic_fetch_add(&total_latency_us, (unsigned long long)c->latency_us);
        } else {
            char header[512];
            int len = snprintf(header, sizeof(header),
                "OK READ request=%llu file=%s bytes=%zu latency_us=%ld\n",
                (unsigned long long)c->request_id, c->filename, c->data_len, c->latency_us);
            send(c->client_fd, header, (size_t)len, MSG_NOSIGNAL);
            if (c->data_len)
                send(c->client_fd, c->data, c->data_len, MSG_NOSIGNAL);
            send(c->client_fd, "\n", 1, MSG_NOSIGNAL);
            free(c->data);
            atomic_fetch_add(&completed_counter, 1);
            atomic_fetch_add(&total_latency_us, (unsigned long long)c->latency_us);
        }

        free(c);
    }

    return NULL;
}

static int create_server_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, 128) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

int main(int argc, char **argv) {
    int port = SERVER_PORT;
    if (argc == 2) port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port\n");
        return 1;
    }

    signal(SIGINT, stop_server);
    signal(SIGTERM, stop_server);
    signal(SIGPIPE, SIG_IGN);

    if (mkdir(STORAGE_DIR, 0755) < 0 && errno != EEXIST) {
        perror("mkdir storage");
        return 1;
    }

    request_queue_init(&request_q);
    completion_queue_init(&completion_q);

    listen_fd = create_server_socket(port);
    if (listen_fd < 0) return 1;

    pthread_t submitter, responder;
    if (pthread_create(&submitter, NULL, io_submitter, NULL) != 0) {
        perror("pthread_create");
        close(listen_fd);
        return 1;
    }
    if (pthread_create(&responder, NULL, response_worker, NULL) != 0) {
        perror("pthread_create");
        running = 0;
        request_queue_stop(&request_q);
        pthread_join(submitter, NULL);
        close(listen_fd);
        return 1;
    }

    printf("Asynchronous File Processing Server listening on port %d\n", port);
    printf("Storage directory: %s\n", STORAGE_DIR);
    printf("Using io_uring for asynchronous file read/write.\n");

    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            if (!running) break;
            perror("accept");
            continue;
        }

        int *fd_arg = malloc(sizeof(int));
        if (!fd_arg) {
            close(client_fd);
            continue;
        }
        *fd_arg = client_fd;

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, fd_arg) != 0) {
            perror("pthread_create client");
            close(client_fd);
            free(fd_arg);
            continue;
        }
        pthread_detach(tid);
    }

    running = 0;
    request_queue_stop(&request_q);
    completion_queue_stop(&completion_q);

    pthread_join(submitter, NULL);
    pthread_join(responder, NULL);

    if (listen_fd >= 0) close(listen_fd);
    printf("Server stopped.\n");
    return 0;
}
