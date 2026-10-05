#include "server.h"
#include <stdlib.h>
#include <string.h>

void request_queue_init(request_queue_t *q) {
    memset(q, 0, sizeof(*q));
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}

void request_queue_push(request_queue_t *q, request_t *req) {
    req->next = NULL;
    pthread_mutex_lock(&q->mutex);
    if (q->tail) q->tail->next = req;
    else q->head = req;
    q->tail = req;
    q->size++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}

request_t *request_queue_pop(request_queue_t *q) {
    pthread_mutex_lock(&q->mutex);
    while (!q->head && !q->stopped)
        pthread_cond_wait(&q->not_empty, &q->mutex);

    if (!q->head) {
        pthread_mutex_unlock(&q->mutex);
        return NULL;
    }

    request_t *req = q->head;
    q->head = req->next;
    if (!q->head) q->tail = NULL;
    q->size--;
    pthread_mutex_unlock(&q->mutex);
    return req;
}

void request_queue_stop(request_queue_t *q) {
    pthread_mutex_lock(&q->mutex);
    q->stopped = 1;
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}

void completion_queue_init(completion_queue_t *q) {
    memset(q, 0, sizeof(*q));
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}

void completion_queue_push(completion_queue_t *q, completion_t *c) {
    c->next = NULL;
    pthread_mutex_lock(&q->mutex);
    if (q->tail) q->tail->next = c;
    else q->head = c;
    q->tail = c;
    q->size++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}

completion_t *completion_queue_pop(completion_queue_t *q) {
    pthread_mutex_lock(&q->mutex);
    while (!q->head && !q->stopped)
        pthread_cond_wait(&q->not_empty, &q->mutex);

    if (!q->head) {
        pthread_mutex_unlock(&q->mutex);
        return NULL;
    }

    completion_t *c = q->head;
    q->head = c->next;
    if (!q->head) q->tail = NULL;
    q->size--;
    pthread_mutex_unlock(&q->mutex);
    return c;
}

void completion_queue_stop(completion_queue_t *q) {
    pthread_mutex_lock(&q->mutex);
    q->stopped = 1;
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}

long elapsed_us(const struct timespec *start, const struct timespec *end) {
    long sec = end->tv_sec - start->tv_sec;
    long nsec = end->tv_nsec - start->tv_nsec;
    return sec * 1000000L + nsec / 1000L;
}

int safe_filename(const char *name) {
    if (!name || !*name || strlen(name) > MAX_FILENAME) return 0;
    if (strstr(name, "..") || strchr(name, '/') || strchr(name, '\\')) return 0;
    return 1;
}
