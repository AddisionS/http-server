# HTTP Server

A small HTTP server built from scratch in C++ to understand how networking and HTTP work beneath the abstractions.

This project is being developed incrementally rather than following a step-by-step tutorial. The goal is to understand **why** each part exists, not just make it work.

---

## Current Progress

### Phase 1 — TCP + HTTP Request Parsing ✅

The server currently:

- Creates an IPv4 TCP socket
- Configures `SO_REUSEADDR`
- Binds to `0.0.0.0:8080`
- Listens for incoming TCP connections
- Accepts client connections
- Identifies the client's IP address
- Receives raw TCP bytes
- Parses the HTTP request line
  - Method
  - Path
  - HTTP version
- Parses HTTP headers into a key-value map
- Closes the client connection

### Current Flow

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
HTTP Parser
  ├── Request Line
  │    ├── Method
  │    ├── Path
  │    └── Version
  │
  └── Headers
       ├── Key
       └── Value
```

---

## Project Structure

```text
http-server/
├── main.cpp
├── http.hpp
└── http.cpp
```

### `main.cpp`

Responsible for the networking layer:

- Creating the socket
- Binding
- Listening
- Accepting connections
- Receiving data
- Managing client connections

### `http.hpp`

Contains the HTTP data structures and parser interface.

### `http.cpp`

Contains the HTTP parsing implementation.

---

## Roadmap

### Phase 1 — TCP + HTTP Request Parsing ✅

- [x] Create IPv4 TCP socket
- [x] Configure `SO_REUSEADDR`
- [x] Bind socket
- [x] Listen for connections
- [x] Accept client connections
- [x] Receive TCP data
- [x] Parse HTTP request line
- [x] Parse HTTP headers

### Phase 2 — HTTP Responses

- [ ] Build HTTP response structure
- [ ] Generate status line
- [ ] Generate response headers
- [ ] Generate response body
- [ ] Return a valid `200 OK` response
- [ ] Test with `curl`

### Phase 3 — Routing

- [ ] Implement basic routing
- [ ] Support different HTTP methods
- [ ] Handle `404 Not Found`
- [ ] Handle `405 Method Not Allowed`

### Phase 4 — Request Bodies

- [ ] Parse `Content-Length`
- [ ] Receive partial HTTP requests
- [ ] Handle request bodies
- [ ] Support basic `POST` requests

### Phase 5 — Static Files

- [ ] Serve files from disk
- [ ] Determine basic MIME types
- [ ] Handle missing files

### Phase 6 — Concurrency

- [ ] Understand blocking vs non-blocking sockets
- [ ] Support multiple clients
- [ ] Explore threads
- [ ] Explore event-driven I/O
- [ ] Explore `select()`
- [ ] Explore `poll()`
- [ ] Explore `epoll()`

### Phase 7 — HTTP/1.1

- [ ] Persistent connections
- [ ] Proper request boundaries
- [ ] Connection management
- [ ] More complete HTTP/1.1 support

---

## Architecture

The project is being built in layers rather than treating HTTP as a black box.

```text
┌──────────────────────┐
│      HTTP Layer      │
│                      │
│ Request Parsing      │
│ Routing              │
│ Response Generation  │
└──────────┬───────────┘
           │
┌──────────▼───────────┐
│      TCP Layer       │
│                      │
│ socket()             │
│ bind()               │
│ listen()             │
│ accept()             │
│ recv()               │
│ send()               │
└──────────┬───────────┘
           │
┌──────────▼───────────┐
│        Kernel        │
│                      │
│ TCP/IP Stack         │
│ Socket Management    │
│ Network Interfaces   │
└──────────────────────┘
```

The objective is to understand what actually happens between:

```text
Browser
   ↓
HTTP
   ↓
TCP
   ↓
Kernel
   ↓
Socket
   ↓
Our Server
   ↓
HTTP Response
```

---

## HTTP Request Parsing

A basic HTTP request looks like:

```text
GET /hello HTTP/1.1
Host: localhost:8080
Connection: close
User-Agent: curl

```

The request is divided into:

```text
Request Line
     ↓
Headers
     ↓
Blank Line
     ↓
Optional Body
```

The request line:

```text
GET /hello HTTP/1.1
```

is parsed into:

```text
Method  → GET
Path    → /hello
Version → HTTP/1.1
```

Headers are stored as key-value pairs:

```text
Host: localhost:8080
Connection: close
```

becomes conceptually:

```text
headers["Host"]       → "localhost:8080"
headers["Connection"] → "close"
```

---

## Building

This project currently targets Linux/POSIX networking APIs.

### Compile

```bash
g++ -Wall -Wextra -pedantic main.cpp http.cpp -o server
```

### Run

```bash
./server
```

The server listens on:

```text
0.0.0.0:8080
```

---

## Testing

### Test TCP Connectivity

Using netcat:

```bash
printf "hello" | nc localhost 8080
```

This was used during the initial TCP implementation to verify that bytes could travel:

```text
Client
  ↓
TCP
  ↓
Server
  ↓
recv()
  ↓
send()
  ↓
Client
```

### Test HTTP Request Parsing

Send a raw HTTP request:

```bash
printf 'GET /hello HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\nUser-Agent: nc\r\n\r\n' | nc localhost 8080
```

The server should parse and display:

```text
Method: GET
Path: /hello
Version: HTTP/1.1
```

along with the parsed headers.

---

## Development Philosophy

This project is intentionally being built without a step-by-step tutorial.

When something is unknown:

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

The goal is not to blindly reproduce an existing HTTP server.

The goal is to understand the systems underneath it and gradually build the abstractions ourselves.

---

## Learning Goals

By the end of the project, I want to understand:

- How sockets work
- How the OS manages file descriptors
- How TCP connections are established and managed
- How `bind()`, `listen()` and `accept()` interact
- How data moves between the kernel and user space
- Why TCP is a byte stream
- Why `recv()` does not necessarily return a complete request
- How HTTP is layered on top of TCP
- How HTTP requests are parsed
- How HTTP responses are constructed
- How routing works
- How concurrent servers handle multiple clients
- How blocking and non-blocking I/O differ
- How event-driven networking works
- What abstractions frameworks normally hide

---

## Status

🚧 **Phase 1 Complete**

The TCP layer and basic HTTP request parsing are working.

The server can currently accept a TCP connection, receive an HTTP request, parse its request line and headers, and represent the result as a structured `Request`.

### Next Milestone

**Generate the first real HTTP response.**

```text
HTTP Request
     ↓
Parse
     ↓
Request
     ↓
Generate Response
     ↓
HTTP Response
```