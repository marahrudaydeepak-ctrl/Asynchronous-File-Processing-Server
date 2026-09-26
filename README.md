# Asynchronous File Processing Server

## Project Overview

The **Asynchronous File Processing Server** is a C-based client-server application developed on Ubuntu/Linux.

The server accepts multiple client connections and processes file operations asynchronously. Client requests are placed into a synchronized request queue and processed by a worker thread using POSIX Asynchronous I/O (AIO).

The system also maintains completion information and measures request latency and server throughput.

## Objectives

- Accept multiple concurrent client connections.
- Process multiple file read and write requests.
- Use asynchronous file I/O.
- Maintain a request queue for pending operations.
- Maintain completion information for completed operations.
- Synchronize shared data using pthread mutexes and condition variables.
- Monitor multiple network clients using poll().
- Record request and completion timestamps.
- Calculate request latency and server throughput.

## Technologies Used

- Programming Language: C
- Operating System: Ubuntu/Linux
- Compiler: GCC
- Networking: TCP Sockets
- I/O Multiplexing: poll()
- Asynchronous I/O: POSIX AIO
- Concurrency: POSIX Threads (pthread)
- Synchronization: Mutex and Condition Variable

## Supported Client Commands

### READ
```
READ input.txt
```

### WRITE
```
WRITE test.txt|Hello from the client
```

### STATS
```
STATS
```

### QUIT
```
QUIT
```

## Requirements

Install GCC and Netcat on Ubuntu:

```bash
sudo apt update
sudo apt install gcc netcat-openbsd
```

## Compilation

```bash
cd ~/async_file_server
gcc server.c -o server -pthread -lrt
```

## Create a Test File

```bash
echo "Hello from asynchronous file server" > input.txt
cat input.txt
```

## Running the Server

```bash
./server
```

The server listens on TCP port **9090**.

## Connecting Multiple Clients

Open separate Ubuntu terminals and run:

```bash
nc 127.0.0.1 9090
```

Example Client 1:

```
WRITE client1.txt|Hello from Client 1
READ client1.txt
```

Example Client 2:

```
WRITE client2.txt|Hello from Client 2
READ client2.txt
```

Example Client 3:

```
WRITE client3.txt|My name is Deepak
READ client3.txt
```

Then use:

```
STATS
```

to view server performance statistics.

## Operating System Concepts Demonstrated

- TCP socket programming
- Concurrent client handling
- poll() I/O multiplexing
- POSIX threads
- Request and completion queues
- Mutexes and condition variables
- POSIX asynchronous I/O
- File descriptors and file operations
- Request latency measurement
- Throughput measurement

## Architecture

```
Clients
   |
 TCP sockets
   |
 poll()
   |
Request Queue
   |
Worker Thread
   |
POSIX AIO
   |
Completion Handling
   |
Client Response
```

## Performance Metrics

Request latency:

```
Latency = Completion Time - Request Time
```

Throughput:

```
Throughput = Completed Requests / Elapsed Time
```

## Verification

After WRITE operations:

```bash
ls -lh
cat client1.txt
cat client2.txt
cat client3.txt
```

## Project Demonstration Flow

1. Compile the server.
2. Start the server.
3. Connect three clients.
4. Send WRITE requests.
5. Send READ requests.
6. Observe server request/completion logs.
7. Run STATS.
8. Verify generated files.
9. Send QUIT from each client.
10. Stop the server with Ctrl+C.

## Conclusion

This project demonstrates concurrent network programming and asynchronous file processing in C on Ubuntu/Linux. It combines TCP sockets, poll(), POSIX threads, synchronization primitives, request/completion management, POSIX AIO, and performance monitoring in one application.
