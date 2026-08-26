# HTTP Server in C++

A lightweight HTTP/1.1 server built from scratch in C++ using Linux sockets and `epoll`.

The goal of this project was to understand how an HTTP server works underneath frameworks and web servers.

## Features

- TCP socket-based HTTP server
- HTTP/1.1 request parsing
- HTTP response serialization
- Non-blocking sockets
- `epoll` event loop
- Multiple simultaneous clients
- Per-client request buffering
- Partial TCP reads
- Multiple requests on the same connection
- HTTP keep-alive
- `Connection: close`
- Partial response writes
- `EPOLLOUT` handling
- Basic routing
- `GET`, `POST`, `PUT`, `DELETE`
- `400 Bad Request`
- `404 Not Found`
- `405 Method Not Allowed`
- `500 Internal Server Error`
- Basic static file serving through route handlers
- Tested through `curl`, `nc`, and a browser
- Can be exposed temporarily through a Cloudflare Quick Tunnel

## Architecture

```text
                    Browser / Client
                           │
                           ▼
                    TCP connection
                           │
                           ▼
                    Non-blocking socket
                           │
                           ▼
                         epoll
                           │
              ┌────────────┴────────────┐
              │                         │
           EPOLLIN                   EPOLLOUT
              │                         │
              ▼                         ▼
        Receive request          Send response
              │
              ▼
        Request buffer
              │
              ▼
        HTTP parser
              │
              ▼
           Router
              │
              ▼
          Handler
              │
              ▼
          Response
              │
              ▼
      HTTP serialization
              │
              ▼
       Response buffer
              │
              ▼
            TCP
```

## Project Structure

```text
http-server/
├── main.cpp
├── server.cpp
├── server.hpp
├── http.cpp
├── http.hpp
└── public/
    ├── index.html
    └── style.css
```

### `main.cpp`

Public-facing application code.

```cpp
Server server(8080);

server.get("/hello", handler);
server.post("/users", handler);
server.put("/users", handler);
server.del("/users", handler);

server.run();
```

### `server.cpp / server.hpp`

Contains the networking layer:

- socket creation
- `bind`
- `listen`
- `accept`
- non-blocking sockets
- `epoll`
- client state
- request/response buffers
- keep-alive
- partial reads/writes
- `EPOLLIN`
- `EPOLLOUT`

### `http.cpp / http.hpp`

Contains:

- `Request`
- `Response`
- HTTP request parsing
- HTTP response serialization
- status codes
- router implementation

## API

Create a server:

```cpp
Server server(8080);
```

### GET

```cpp
server.get(
    "/hello",
    [](const Request&, Response& res) {
        res.status_code = StatusCode::OK;
        res.body = "Hello from API!";
        res.headers["Content-Type"] = "text/plain";
    }
);
```

### POST

```cpp
server.post(
    "/users",
    [](const Request& req, Response& res) {
        res.status_code = StatusCode::OK;
        res.body = "Received: " + req.body;
        res.headers["Content-Type"] = "text/plain";
    }
);
```

### PUT

```cpp
server.put("/users", handler);
```

### DELETE

```cpp
server.del("/users", handler);
```

`del()` is used because `delete` is a C++ keyword.

## Status Codes

| Code | Meaning |
|---|---|
| `200` | OK |
| `400` | Bad Request |
| `404` | Not Found |
| `405` | Method Not Allowed |
| `500` | Internal Server Error |

```text
Bad HTTP request       → 400
Route doesn't exist    → 404
Wrong HTTP method      → 405
Handler throws         → 500
```

## Static Files

Static files can currently be served using normal `GET` handlers.

```cpp
server.get(
    "/",
    [](const Request&, Response& res) {
        res.status_code = StatusCode::OK;
        res.body = readFile("./public/index.html");
        res.headers["Content-Type"] = "text/html";
    }
);
```

And CSS:

```cpp
server.get(
    "/style.css",
    [](const Request&, Response& res) {
        res.status_code = StatusCode::OK;
        res.body = readFile("./public/style.css");
        res.headers["Content-Type"] = "text/css";
    }
);
```

A browser requesting `/` sees:

```html
<link rel="stylesheet" href="/style.css">
```

and automatically makes another HTTP request for `/style.css`.

## Building

From WSL/Linux:

```bash
g++ -Wall -Wextra -pedantic main.cpp server.cpp http.cpp -o server
```

Run:

```bash
./server
```

The server listens on:

```text
http://localhost:8080
```

## Testing

### GET

```bash
printf 'GET /hello HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n' | nc localhost 8080
```

### POST

```bash
printf 'POST /users HTTP/1.1\r\nHost: localhost\r\nContent-Length: 11\r\nConnection: close\r\n\r\nHello World' | nc localhost 8080
```

### 404

```bash
printf 'GET /doesnotexist HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n' | nc localhost 8080
```

### 405

```bash
printf 'POST /hello HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\nConnection: close\r\n\r\n' | nc localhost 8080
```

### 500

```bash
printf 'GET /error HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n' | nc localhost 8080
```

## Keep-Alive

HTTP/1.1 connections are kept alive by default.

Multiple requests can be sent over the same TCP connection:

```bash
printf 'GET /hello HTTP/1.1\r\nHost: localhost\r\n\r\nGET /users HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n' | nc localhost 8080
```

## Concurrency

The server uses Linux `epoll` instead of creating a thread for every connection.

```text
              epoll_wait()
                   │
        ┌──────────┼──────────┐
        ▼          ▼          ▼
      Client 1   Client 2   Client 3
        │          │          │
      ready      ready      ready
```

This allows a single event loop to handle multiple clients without blocking on one connection.

## Public Testing

For temporary external testing, Cloudflare Quick Tunnel can expose the local server:

```bash
cloudflared tunnel --url http://localhost:8080
```

This provides a temporary `trycloudflare.com` URL that forwards requests to the local server.

## Current Limitations

This is a learning-focused HTTP server, not a production web server.

Not currently implemented:

- Dynamic route parameters
- Middleware
- JSON helpers
- Automatic static-file serving
- MIME-type lookup for arbitrary files
- Chunked transfer encoding
- HTTPS/TLS
- HTTP/2
- HTTP/3
- Authentication
- Rate limiting
- Request size limits
- Timeouts
- Production-grade error handling
- Graceful shutdown
- Advanced connection management

## What This Project Taught

The stack was built from the bottom up:

```text
TCP
 ↓
Sockets
 ↓
Non-blocking I/O
 ↓
epoll
 ↓
Connection management
 ↓
HTTP parsing
 ↓
HTTP serialization
 ↓
Routing
 ↓
Application handlers
 ↓
Public C++ API
```

The important part isn't the final number of features.

It's that the abstraction was built after understanding the machinery underneath it.

No framework doing the magic.

Just sockets, bytes, state machines, and a frankly unreasonable amount of debugging.
