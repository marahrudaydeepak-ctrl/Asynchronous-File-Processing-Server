#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 8192

static int send_all(int fd, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static int recv_line(int fd, char *buf, size_t cap) {
    size_t used = 0;
    while (used + 1 < cap) {
        char ch;
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n <= 0) return (int)n;
        buf[used++] = ch;
        if (ch == '\n') break;
    }
    buf[used] = '\0';
    return (int)used;
}

static int connect_to_server(const char *host, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server address: %s\n", host);
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return -1;
    }

    return fd;
}

static void print_read_response(int fd, const char *header) {
    unsigned long long bytes = 0;
    if (sscanf(header, "OK READ %*[^ ] %*[^ ] bytes=%llu", &bytes) != 1) {
        printf("%s", header);
        return;
    }

    printf("%s", header);

    unsigned long long received = 0;
    char buffer[BUFFER_SIZE];

    while (received < bytes) {
        size_t want = bytes - received;
        if (want > sizeof(buffer) - 1) want = sizeof(buffer) - 1;

        ssize_t n = recv(fd, buffer, want, 0);
        if (n <= 0) {
            printf("\nConnection closed while receiving file data.\n");
            return;
        }

        buffer[n] = '\0';
        fwrite(buffer, 1, (size_t)n, stdout);
        received += (unsigned long long)n;
    }

    char newline;
    if (recv(fd, &newline, 1, 0) > 0)
        putchar(newline);
}

int main(int argc, char **argv) {
    const char *host = "127.0.0.1";
    int port = 9090;

    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = atoi(argv[2]);

    int fd = connect_to_server(host, port);
    if (fd < 0) return 1;

    char buffer[BUFFER_SIZE];

    if (recv_line(fd, buffer, sizeof(buffer)) > 0)
        printf("%s", buffer);
    if (recv_line(fd, buffer, sizeof(buffer)) > 0)
        printf("%s", buffer);

    printf("file-server> ");
    fflush(stdout);

    while (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);
        if (send_all(fd, buffer, len) < 0) {
            perror("send");
            break;
        }

        if (strncmp(buffer, "QUIT", 4) == 0) {
            if (recv_line(fd, buffer, sizeof(buffer)) > 0)
                printf("%s", buffer);
            break;
        }

        if (strncmp(buffer, "READ ", 5) == 0) {
            if (recv_line(fd, buffer, sizeof(buffer)) <= 0) break;
            print_read_response(fd, buffer);
        } else {
            if (recv_line(fd, buffer, sizeof(buffer)) <= 0) break;
            printf("%s", buffer);
        }

        printf("file-server> ");
        fflush(stdout);
    }

    close(fd);
    return 0;
}
