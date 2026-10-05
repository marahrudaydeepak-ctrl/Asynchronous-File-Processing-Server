#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#define SERVER_PORT 9090
#define STORAGE_DIR "storage"
#define MAX_FILENAME 255
#define MAX_LINE 4096
#define MAX_DATA 1048576
#define MAX_INFLIGHT 128

typedef enum {
    OP_READ,
    OP_WRITE
} operation_t;

typedef struct request {
    int client_fd;
    operation_t op;
    char filename[MAX_FILENAME + 1];
    char *data;
    size_t data_len;
    struct timespec start_time;
    uint64_t id;
    struct request *next;
} request_t;

typedef struct completion {
    int client_fd;
    operation_t op;
    char filename[MAX_FILENAME + 1];
    char *data;
    size_t data_len;
    int result;
    uint64_t request_id;
    long latency_us;
    struct completion *next;
} completion_t;

typedef struct {
    request_t *head;
    request_t *tail;
    size_t size;
    int stopped;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} request_queue_t;

typedef struct {
    completion_t *head;
    completion_t *tail;
    size_t size;
    int stopped;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} completion_queue_t;

void request_queue_init(request_queue_t *q);
void request_queue_push(request_queue_t *q, request_t *req);
request_t *request_queue_pop(request_queue_t *q);
void request_queue_stop(request_queue_t *q);

void completion_queue_init(completion_queue_t *q);
void completion_queue_push(completion_queue_t *q, completion_t *c);
completion_t *completion_queue_pop(completion_queue_t *q);
void completion_queue_stop(completion_queue_t *q);

long elapsed_us(const struct timespec *start, const struct timespec *end);
int safe_filename(const char *name);

#endif
