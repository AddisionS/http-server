# HTTP Server

A small HTTP server built from scratch in C++ to understand how networking and HTTP work beneath the abstractions.

This project is being developed incrementally rather than following a complete tutorial. The goal is to understand **why** each part exists, not just make it work.

## Current Progress

### TCP Server — Complete

The server currently:

* Creates an IPv4 TCP socket
* Configures `SO_REUSEADDR`
* Binds to `0.0.0.0:8080`
* Listens for incoming TCP connections
* Accepts client connections
* Receives raw TCP bytes
* Echoes received bytes back to the client
* Closes the client connection

Current flow:

```text
Client
  ↓
TCP connection
  ↓
socket()
  ↓
bind()
  ↓
listen()
  ↓
accept()
  ↓
recv()
  ↓
send()
  ↓
close()
```

## Roadmap

The TCP layer is only the foundation. The next stages will build HTTP on top of it:

* [ ] Inspect raw HTTP requests
* [ ] Parse HTTP request lines
* [ ] Parse HTTP headers
* [ ] Generate HTTP responses
* [ ] Implement basic routing
* [ ] Serve static files
* [ ] Handle HTTP status codes
* [ ] Support multiple clients
* [ ] Explore non-blocking I/O
* [ ] Add better observability/debugging
* [ ] Improve HTTP/1.1 support

## Philosophy

This project is intentionally being built without a step-by-step tutorial.

When something is unknown, the approach is:

```text
Don't know
   ↓
Investigate
   ↓
Read documentation
   ↓
Experiment
   ↓
Understand
   ↓
Implement
```

The objective is not to build the most production-ready HTTP server.

The objective is to understand what actually happens between:

```text
Browser → TCP → Kernel → Server → HTTP → Response
```

## Building

This project currently targets Linux/POSIX networking APIs.

Compile with:

```bash
g++ -Wall -Wextra -pedantic main.cpp -o server
```

Run:

```bash
./server
```

The server listens on:

```text
0.0.0.0:8080
```

## Testing

Using netcat:

```bash
printf "hello" | nc localhost 8080
```

The server should echo the received bytes back.

## Status

🚧 **Early development**

Currently functioning as a basic TCP echo server. HTTP functionality has not been implemented yet.
