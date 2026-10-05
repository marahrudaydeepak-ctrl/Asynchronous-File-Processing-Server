# Asynchronous File Processing Server

## Abstract

The **Asynchronous File Processing Server** is a Linux-based Operating Systems and Systems Programming project implemented in C to efficiently handle multiple concurrent file and network requests. In a traditional synchronous server, a request may remain blocked while opening, reading, or writing a file. This can increase waiting time and reduce scalability when many requests arrive together.

This project implements an asynchronous file-processing architecture using TCP sockets, concurrent client handling, a pending request queue, Linux asynchronous I/O, a completion queue, POSIX synchronization, and performance measurement. Incoming requests are received through TCP communication and organized in a thread-safe request queue. File read and write operations are submitted using Linux asynchronous I/O mechanisms, with **io_uring** as the preferred mechanism in the project design. When an operation completes, the result is processed and returned to the appropriate client.

Mutexes and condition variables are used to protect shared request and completion information and to reduce race-condition risks. The server records request and completion timestamps using Linux timing APIs, allowing latency and throughput to be measured. The project therefore demonstrates concurrency, asynchronous I/O, socket programming, queues, synchronization, system calls, file operations, resource management, and performance analysis.

This README is based on the submitted project problem statement for **Operating Systems and Systems Programming (25CS2104E), 2026–27, Term-I, Section 7, Team 23**. The submitted abstract identifies concurrent client handling, asynchronous file operations, request and completion queues, synchronization, latency measurement, and throughput as the main project goals. fileciteturn2file0L13-L26

## Problem Statement

Cloud storage and network file systems may receive many simultaneous requests for opening, reading, writing, and transferring files. Processing these operations synchronously can cause blocking, increased waiting time, and reduced server throughput. Creating a separate blocking thread for every request can also introduce resource overhead as the number of requests grows.

The purpose of this project is to develop a Linux-based asynchronous file-processing server in C that can manage multiple client connections and file operations efficiently. The server maintains request and completion queues, performs file I/O asynchronously, synchronizes access to shared data, and measures request latency and throughput. fileciteturn2file0L27-L37

## Objectives

1. Develop a Linux-based asynchronous file-processing server using C.
2. Accept and manage multiple concurrent client connections.
3. Process file-read and file-write requests asynchronously.
4. Maintain a queue of pending I/O requests.
5. Maintain a completion queue for completed operations and results.
6. Use synchronization mechanisms to safely access shared request data.
7. Avoid unnecessary blocking and improve scalability.
8. Record request start and completion timestamps.
9. Calculate request latency and overall server throughput.
10. Test performance under multiple concurrent client requests. fileciteturn2file0L38-L48

## Key Features

- TCP client-server communication
- Multiple concurrent clients
- Thread-based client handling
- Thread-safe pending request queue
- Linux asynchronous file I/O
- io_uring-based file processing
- Completion processing
- Mutex and condition-variable synchronization
- READ and WRITE file operations
- STATS performance command
- Concurrent load testing
- Latency measurement
- Throughput measurement
- Web demonstration console
- Python gateway and load-testing support

## System Architecture

    Clients
       |
       v
    TCP Socket Server
       |
       v
    Client Handlers / Pthreads
       |
       v
    Pending Request Queue
       |
       v
    Asynchronous I/O Submission
       |
       v
    Linux io_uring
       |
       v
    File Read / Write
       |
       v
    Completion Processing
       |
       v
    Client Response

The proposed methodology follows server initialization, client connection handling, request queuing, asynchronous file processing, completion handling, synchronization, performance monitoring, and testing. fileciteturn2file0L49-L74

## How the System Works

### 1. Server Initialization
The server creates a TCP socket, binds it to a port, and listens for incoming connections.

### 2. Client Connection
A client connects and sends a command. Multiple clients can be active concurrently.

### 3. Request Creation
The server interprets the command and creates a request containing operation type, file name, data, and timing information.

### 4. Request Queue
The request is placed into a thread-safe pending queue. Synchronization protects the shared queue.

### 5. Asynchronous I/O
File read and write operations are submitted to Linux asynchronous I/O, preferably through io_uring.

### 6. Completion
When the operation finishes, completion information is obtained and associated with the original request.

### 7. Response
The result is processed and returned to the appropriate client.

### 8. Performance Measurement
Request and completion timestamps are used to calculate latency and throughput.

## Operating Systems Concepts

### Concurrency
Multiple clients and requests can be active during the same period.

### POSIX Threads
Pthreads provide concurrent client handling and background processing.

### Synchronization
Mutexes and condition variables protect shared request and completion data.

### Producer-Consumer Queues
Incoming requests can be treated as produced work items while processing components consume requests from the queue.

### Asynchronous I/O
io_uring allows file operations to proceed asynchronously rather than continuously blocking the server.

### Socket Programming
TCP sockets provide reliable client-server communication.

### File Descriptors
Linux represents sockets and opened files using file descriptors.

### System Calls
Linux/POSIX APIs are used for networking, file processing, timing, and synchronization.

### Race-Condition Prevention
Synchronization helps prevent unsafe simultaneous modification of shared data.

### Resource Management
The server manages connections, files, threads, queues, and asynchronous operations.

The submitted documentation specifically identifies socket APIs, io_uring, POSIX threads, mutexes, condition variables, file system calls, and clock_gettime() as relevant APIs and mechanisms. fileciteturn2file0L76-L92

## Linux APIs and System Calls

### Socket APIs
- socket()
- bind()
- listen()
- accept()

### File APIs
- open()
- read()
- write()
- close()

### Thread and Synchronization APIs
- pthread
- pthread_mutex
- pthread_cond

### Timing API
- clock_gettime()

## Technologies Used

| Technology | Purpose |
|---|---|
| C | Core server and client implementation |
| Linux / Ubuntu | Development and execution platform |
| GCC | Compilation |
| POSIX Threads | Concurrent processing |
| pthread mutex | Shared-data synchronization |
| pthread condition variables | Queue coordination |
| TCP Sockets | Network communication |
| io_uring | Asynchronous file I/O |
| clock_gettime() | High-resolution timing |
| Python 3 | Load testing and gateway support |
| Make | Build automation |
| HTML/CSS/JavaScript | Web demonstration interface |

## Installation on Ubuntu

    sudo apt update
    sudo apt install build-essential liburing-dev python3

Verify the installation:

    gcc --version
    python3 --version
    pkg-config --modversion liburing

## Build

Recommended:

    make

Manual server compilation:

    gcc -Wall -Wextra -O2 -std=c11 -D_GNU_SOURCE -o server server.c queue.c -luring -lpthread

Client compilation:

    gcc -Wall -Wextra -O2 -std=c11 -D_GNU_SOURCE -o client client.c

## Run

Terminal 1:

    ./server

The default server port is **9090**.

Terminal 2:

    ./client

## Client Commands

### WRITE

    WRITE notes.txt Hello from the asynchronous server

Writes data to a file through the server.

### READ

    READ notes.txt

Requests file contents from the server.

### STATS

    STATS

Displays server performance information.

### QUIT

    QUIT

Closes the client session.

## Storage

Processed files are stored inside the **storage/** directory. This provides a simple local storage area for demonstrating file-processing operations.

## Performance Measurement

Performance evaluation is an important part of the project.

### Latency

    latency = completion_time - request_start_time

Latency represents the time taken between request processing and completion.

### Throughput

    throughput = completed_operations / elapsed_time

Throughput represents the number of completed operations over a period of time.

The project methodology specifies recording request arrival, processing, and completion times to calculate latency, completed requests per second, and overall throughput. fileciteturn2file0L68-L74

## Concurrent Load Testing

Start the server first:

    ./server

Run a test with 10 clients:

    python3 load_test.py 10 20

Run a larger test:

    python3 load_test.py 50 20

The load test is intended to evaluate server behavior when multiple clients perform repeated file operations.

## Example Demonstration Flow

1. Start Ubuntu/Linux.
2. Install the required packages.
3. Build the project with make.
4. Start ./server.
5. Start ./client in another terminal.
6. Execute WRITE.
7. Execute READ.
8. Execute STATS.
9. Run the concurrent load test.
10. Observe completed requests, latency, and throughput.
11. Close the client and stop the server after testing.

## Project Structure

    Asynchronous-File-Processing-Server/
    |-- server.c
    |-- server.h
    |-- client.c
    |-- queue.c
    |-- gateway.py
    |-- load_test.py
    |-- index.html
    |-- Makefile
    |-- PROJECT_REPORT.md
    |-- README.md
    |-- VIVA.md
    |-- test_commands.txt
    |-- .gitignore
    `-- storage/

## File Descriptions

**server.c** - Main server implementation containing networking and request-processing logic.

**server.h** - Shared server declarations and definitions.

**client.c** - Command-line client for communicating with the server.

**queue.c** - Request queue implementation.

**gateway.py** - Python bridge between the web interface and TCP server.

**load_test.py** - Utility for generating concurrent client requests and evaluating performance.

**index.html** - Web console for demonstration and monitoring.

**Makefile** - Automates compilation.

**PROJECT_REPORT.md** - Additional project documentation.

**VIVA.md** - Viva preparation material.

**test_commands.txt** - Testing and command reference.

## Web Console

The repository contains a web console and Python gateway for demonstration purposes. The interface can organize demonstrations around server statistics, stored files, file operations, activity information, load testing, and processing-pipeline visualization.

The web layer is a demonstration interface; the Linux C server remains the core Operating Systems and Systems Programming implementation.

## Advantages

1. Supports multiple concurrent client connections.
2. Separates incoming requests from file processing through queues.
3. Uses asynchronous file I/O to reduce unnecessary blocking.
4. Protects shared data with synchronization mechanisms.
5. Provides latency and throughput measurements.
6. Demonstrates several important Linux and Operating Systems concepts in one project.
7. Includes command-line testing and concurrent load testing.

## Limitations

This is an academic Operating Systems and Systems Programming project rather than a production cloud-storage platform. The primary focus is concurrency, asynchronous file I/O, synchronization, socket programming, queues, system calls, and performance measurement.

A production deployment would require additional capabilities such as authentication, encrypted communication, advanced access control, distributed storage, replication, fault tolerance, comprehensive monitoring, and large-scale deployment management.

## Future Enhancements

1. Fully event-driven network I/O using io_uring.
2. Authentication and authorization.
3. Secure encrypted client-server communication.
4. User-specific file permissions.
5. Distributed file storage.
6. File replication and fault tolerance.
7. Persistent metadata management.
8. Advanced monitoring dashboards.
9. Configurable server parameters.
10. Larger and more detailed benchmarking.
11. Automatic recovery from failed operations.
12. Improved logging and diagnostics.
13. Containerized deployment.
14. Cloud deployment.
15. Advanced performance visualization.
16. Improved handling of large file transfers.

## Expected Outcome

The expected outcome is a functional asynchronous file-processing server capable of handling multiple client requests concurrently. It performs file read and write operations asynchronously, maintains request and completion queues, synchronizes shared data safely, and measures request latency and server throughput. fileciteturn2file0L109-L113

## Team

| Roll Number | Student Name | Responsibility |
|---|---|---|
| 2520030138 | Hruday Deepak | Server design, socket programming and client connection handling |
| 2520030135 | Jaiswal Reddy | Asynchronous file I/O, request queues, synchronization and performance testing |

The team details and responsibilities are taken from the submitted project documentation. fileciteturn2file0L93-L100

## Course Information

**Course:** Operating Systems and Systems Programming  
**Course Code:** 25CS2104E  
**Academic Year:** 2026–27  
**Term:** I  
**Section:** 7  
**Team:** 23  
**Project Title:** Asynchronous File Processing Server

## Repository

GitHub repository:

https://github.com/marahrudaydeepak-ctrl/Asynchronous-File-Processing-Server

## Conclusion

The Asynchronous File Processing Server demonstrates how operating-system mechanisms can be integrated into a practical Linux application. The project combines TCP socket communication, concurrent client handling, request queues, asynchronous file I/O, completion processing, POSIX synchronization, Linux file operations, and high-resolution timing.

The architecture separates network communication from request management and file processing. Requests can be queued, asynchronous file operations can be submitted, and completed operations can be processed before results are returned to clients. This makes the project useful for understanding how concurrency, synchronization, asynchronous I/O, networking, and resource management work together.

Performance measurement further strengthens the project by allowing request latency and throughput to be observed under different workloads. The included load-testing approach makes it possible to evaluate the behavior of the server with multiple simultaneous clients.

Overall, the project provides a practical demonstration of the Operating Systems and Systems Programming concepts described in the submitted project problem statement and provides a foundation for future development toward more advanced and scalable event-driven file-processing systems.
