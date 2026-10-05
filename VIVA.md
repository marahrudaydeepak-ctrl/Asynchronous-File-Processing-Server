# Viva / Demonstration Questions

## 1. Why is the project asynchronous?
File read/write operations are submitted through Linux io_uring, so the server does not wait synchronously for each file transfer to finish.

## 2. Why use a request queue?
It decouples client request arrival from file-I/O processing and provides a synchronized producer-consumer structure.

## 3. Why use a completion queue?
Completed I/O results are separated from request submission and can be processed by a response worker.

## 4. Why use mutexes?
Multiple threads access the shared queues, so mutexes protect queue state from race conditions.

## 5. Why use condition variables?
A consumer can sleep while a queue is empty and wake when a producer adds work.

## 6. What is io_uring?
It is a Linux asynchronous I/O interface using submission queues and completion queues.

## 7. What is a file descriptor?
It is an integer handle used by Linux to represent an open file, socket, or other resource.

## 8. How is latency calculated?
The server records a monotonic start timestamp when the request is created and another timestamp when the I/O completion is processed.

## 9. How is throughput measured?
The load-test program divides the total number of completed file operations by total wall-clock execution time.

## 10. What OS concepts are demonstrated?
Concurrency, synchronization, producer-consumer queues, socket programming, system calls, asynchronous I/O, file descriptors, timing, and resource management.
