# Project Report — Asynchronous File Processing Server

## 1. Project title
Asynchronous File Processing Server

## 2. Problem
A synchronous file server can block while handling file operations. The submitted abstract identifies the need to handle multiple simultaneous file requests efficiently and avoid the overhead of a blocking thread for every operation.

## 3. Proposed solution
The implementation uses:
- TCP sockets for client/server communication.
- A pthread-based client connection layer.
- A thread-safe pending request queue.
- Linux io_uring for asynchronous file read/write submission.
- A completion queue for completed operations.
- Mutexes and condition variables for queue synchronization.
- clock_gettime(CLOCK_MONOTONIC) for latency measurements.
- Atomic counters for completed/failed requests and total latency.

## 4. Request flow
1. Client connects through TCP.
2. Client sends `READ`, `WRITE`, `STATS`, or `QUIT`.
3. A client handler parses the request.
4. File requests enter the pending request queue.
5. The I/O submitter removes requests from the queue.
6. The submitter prepares an io_uring read/write operation.
7. Linux completes the operation and produces a CQE.
8. The result is placed in the completion queue.
9. The response worker sends the result to the correct client.
10. Completion time is compared with request start time.

## 5. Data structures
### Request
Stores:
- client socket
- operation type
- filename
- write data
- data length
- request timestamp
- request ID

### Completion
Stores:
- client socket
- operation type
- filename
- result code
- returned data
- request ID
- latency

### Queues
Both queues are FIFO linked lists protected by:
- `pthread_mutex_t`
- `pthread_cond_t`

## 6. Performance
For a request:

`latency_us = completion_time - request_start_time`

The server's `STATS` command reports:
- completed requests
- failed requests
- average latency

`load_test.py` generates concurrent clients and calculates:

`throughput = total file operations / wall-clock execution time`

## 7. Limitations
- The socket layer uses a pthread per connected client; it does not create a new thread per file operation.
- File opening and metadata checks are performed before submitting the data transfer to io_uring.
- The protocol is intentionally text-based for easy demonstration.
- This is an academic implementation, not a production cloud storage service.

## 8. Future enhancements
- Use io_uring for socket accept/read/write as well as file I/O.
- Add authentication and access control.
- Add binary-safe framed protocol.
- Add configurable worker/in-flight limits.
- Add persistent performance logs and CSV output.
- Add graceful client-session tracking.
