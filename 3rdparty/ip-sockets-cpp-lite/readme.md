
# [ip-sockets-cpp-lite](https://github.com/biaks/ip-sockets-cpp-lite) 🔌

[![Build examples](https://github.com/biaks/ip-sockets-cpp-lite/actions/workflows/cmake.yml/badge.svg)](https://github.com/biaks/ip-sockets-cpp-lite/actions/workflows/cmake.yml)
[![Build multi-platform](https://github.com/biaks/ip-sockets-cpp-lite/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/biaks/ip-sockets-cpp-lite/actions/workflows/cmake-multi-platform.yml)


**Simple. Lightweight. Cross-platform.**

**ip-sockets-cpp-lite** is a fast, header-only, dependency-free, cross platform C++ library that makes working with UDP/TCP sockets and IPv4/IPv6 addresses straightforward and predictable.

No more endless `getaddrinfo` calls, confusing `sockaddr_in` / `sockaddr_in6` structures, or platform-specific workarounds.

---

## ✨ Why ip-sockets-cpp-lite?

* 📦 **Header-only**            - just copy headers into your project
* 🎯 **Zero dependencies**      - only the C++ standard library
* 🖥️ **Cross-platform**         - Linux, Windows, macOS
* 🚀 **Minimal learning curve** - up and running in minutes
* 🪶 **Lightweight**              - no hidden magic
* 🔒 **Predictable behavior**   - explicit control over sockets
* 📋 **Readable errors**        - clear error codes instead of errno confusion

---

## 🚀 Quick Example

```cpp
// Send a UDP packet to a hostname
udp_socket_t<v4, socket_type_e::client> sock (log_e::debug);
ip4_t   ip   = sock.resolve("localhost", nullptr, log_e::debug);
addr4_t addr = {ip, 8080};
sock.open(addr);
sock.send("Hello!", 6);

// Send a TCP message to an IPv6 address
tcp_socket_t<v6, socket_type_e::client> sock6 (log_e::debug);
sock6.open("[::1]:8081");
sock6.send("Hello!", 6);

// Zero-copy overlay on raw memory
uint8_t raw_data[10] = {0x45,0x00,0x00, 0x7f,0x00,0x00,0x01, 0xa8,0x00,0x00};
ip4_t& reint_ip = *reinterpret_cast<ip4_t*>(&raw_data[3]);
std::cout << "ip from raw data: " << reint_ip << std::endl;
```
The log in the console will be like this:
```
udp<ip4,client>: [static].resolve() [undefined -> localhost] DNS resolution success, resolved to '127.0.0.1'
udp<ip4,client>: [3].open() [127.0.0.1:50902 <> 127.0.0.1:8080] success
udp<ip4,client>: [3].send() [127.0.0.1:50902 -> 127.0.0.1:8080] sended 6 bytes
tcp<ip6,client>: [4].open() [[::1]:39129 <> [::1]:8081] success
tcp<ip6,client>: [4].send() [[::1]:39129 -> [::1]:8081] sended 6 bytes
ip from raw data: 127.0.0.1
```

---

## 🧠 Design Philosophy

This library provides **blocking I/O sockets with configurable timeouts** — the simplest model that works for most real-world applications.

Keep it simple. Keep it portable.

---

## 🔧 Features

### 🌐 IP Address Handling (`ip_address.h`)

* Unified IPv4 and IPv6 API
* String ↔ binary conversion
* Network prefixes and masks
* IPv4-mapped IPv6 support
* Address + port as a single type
* Zero-copy overlays on existing buffers
* Flexible parsing (hex, decimal, dotted)
* Rich constructors from strings, numbers, and byte arrays

### 📡 UDP Sockets (`udp_socket.h`)

* Consistent API across platforms
* Client and server modes
* Blocking I/O with timeouts
* Configurable logging
* Clear states and error codes
* **RAW mode** — send hand-crafted IP packets with custom headers (IP_HDRINCL)

### 🔌 TCP Sockets (`tcp_socket.h`)

* Simple accept workflow
* Automatic connection lifecycle
* API consistent with UDP sockets
* **`std::iostream` interface** — use `<<`, `>>`, `std::getline` over TCP

---

## 📋 Requirements

* C++11 or newer
* Standard library
* Nothing else

---

## 🏁 Getting Started

### 🏗️ Integration

**Option 1 — Copy headers**

from /include folder copy:

* [`ip_address.h`](include/ip_address.h)
* [`udp_socket.h`](include/udp_socket.h)
* [`tcp_socket.h`](include/tcp_socket.h)

**Option 2 — Use CMake**

```cmake
add_subdirectory(ip-sockets-cpp-lite)
target_link_libraries(your_app ip-sockets-cpp-lite)
```

Then include what you need:

```cpp
#include "ip_address.h"  // work only with ipv4/ipv6 addresses
#include "udp_socket.h"  // work with UDP ipv4/ipv6 client/server sockets
#include "tcp_socket.h"  // work with TCP ipv4/ipv6 client/server sockets + tcp_stream_t
```

### 🌐 Working with IP Addresses

#### Create from different formats
```cpp
// IPv4 addresses - all the common formats
ip4_t ip0 = "192.168.1.1";       // from string
ip4_t ip1 = 0xc0a80101;          // from uint32
ip4_t ip2 = {192, 168, 1, 1};    // from bytes
ip4_t ip3 = "0xc0a80101";        // from hex string
ip4_t ip4 = "3232235777";        // from decimal string
ip4_t ip5 = "127.1";             // 127.0.0.1 (missing bytes are zero)
// IPv6 addresses - all the common formats
ip6_t ip6 = "fe80::";            // link-local address
ip6_t ip7 = "::1";               // loopback
ip6_t ip8 = "64:ff9b::10.0.0.7"; // IPv4-translated (NAT64)
ip6_t ip9 = "11.0.0.1";          // auto IPv4-mapped to ::ffff:11.0.0.1
// Parsing of all variants of input strings always occurs in one pass
```

#### Zero-copy overlay on existing memory
```cpp
uint8_t raw_data[10] = {0x45,0x00,0x00, 0x7f,0x00,0x00,0x01, 0xa8,0x00,0x00};
// Treat memory region as ip4_t without copying!
ip4_t& reint_ip = *reinterpret_cast<ip4_t*>(&raw_data[3]);
std::cout << reint_ip << std::endl; // prints "127.0.0.1"
```

#### Network operations
```cpp
ip4_t ip = "192.168.1.100";
ip4_t mask(24);                         // 255.255.255.0 from prefix
ip4_t network = ip & mask;              // 192.168.1.0

ip6_t ipv6 = "2001:db8::1";
ip6_t ipv6_from_v4 = ip4_t("10.0.0.1"); // ::ffff:10.0.0.1 (IPv4-mapped)
```

#### Address + port
```cpp
addr4_t server  = "192.168.1.100:8080";  // IP and port together
addr6_t server6 = "[2001:db8::1]:8080";
std::cout << server;                     // prints "192.168.1.100:8080"
```

#### Two Ways to Use IP and Address Types
```cpp
// Direct types (when IP version is known)
ip4_t ip4     = "192.168.1.1";      // IPv4
ip6_t ip6     = "2001:db8::1";      // IPv6
addr4_t addr4 = "192.168.1.1:8080"; // IPv4 + port
addr6_t addr6 = "[::1]:8080";       // IPv6 + port

// Generic types (template-friendly)
ip_t<v4> ip4     = "192.168.1.1";      // same as ip4_t
ip_t<v6> ip6     = "2001:db8::1";      // same as ip6_t
addr_t<v4> addr4 = "192.168.1.1:8080"; // same as addr4_t
addr_t<v6> addr6 = "[::1]:8080";       // same as addr6_t

// Perfect for templates!
template <ip_type_e Type>
void print_endpoint(const addr_t<Type>& endpoint) {
    std::cout << "Connecting to: " << endpoint << '\n';
}
```

### 📡 UDP Client Example
```cpp
udp_socket_t<v4, socket_type_e::client> sock;

if (sock.open("192.168.1.100:8080") == no_error) {
    sock.send("Hello, world!", 13);

    char buffer[1024];
    int bytes = sock.recv(buffer, sizeof(buffer));
    if (bytes > 0)
        std::cout << "Received: " << std::string(buffer, bytes) << '\n';
}
```

### 🖥️ UDP Server Example
```cpp
udp_socket_t<v4, socket_type_e::server> server;

if (server.open("0.0.0.0:8080") == no_error) {

    char    buffer[1024];
    addr4_t client;

    while (true) {
        int bytes = server.recvfrom(buffer, sizeof(buffer), client);
        if (bytes > 0) {
            std::cout << "Received from " << client << ": "
                      << std::string(buffer, bytes) << '\n';

            server.sendto("OK", 2, client);
        }
    }
}
```

### 🔌 TCP Echo Server
```cpp
tcp_socket_t<v4, socket_type_e::server> server;

if (server.open("0.0.0.0:8080") == no_error) {

    std::cout << "TCP server listening on port 8080\n";

    while (true) {
        addr4_t client_addr;
        tcp_socket_t<v4, socket_type_e::client> client = server.accept(client_addr);

        if (client.state != state_e::state_opened)
            continue;

        std::cout << "Client connected: " << client_addr << '\n';

        char buffer[1024];
        int bytes = client.recv(buffer, sizeof(buffer));

        if (bytes > 0)
            client.send(buffer, bytes); // echo back

        client.close();
    }
}
```

### 🔌 TCP Stream — iostream over TCP

`tcp_stream_t` wraps a TCP socket into a standard `std::iostream`, so you can use `<<`, `>>`, `std::getline` and all familiar C++ stream operations directly over a network connection.

#### Client: send and receive with `<<` and `>>`
```cpp
tcp_socket_t<v4, socket_type_e::client> sock;
sock.open("192.168.1.100:8080");

// Wrap opened socket into a stream
tcp_stream_t<v4, socket_type_e::client> stream (std::move(sock));

// Send data — just like std::cout
stream << "Hello " << 42 << std::endl;

// Read response — just like std::cin
std::string line;
std::getline(stream, line);
std::cout << "Server replied: " << line << '\n';
```

#### Server: accept and communicate via streams
```cpp
tcp_socket_t<v4, socket_type_e::server> server;
server.open("0.0.0.0:8080");

addr4_t client_addr;
auto accepted = server.accept(client_addr);

// Wrap accepted connection into a stream
tcp_stream_t<v4, socket_type_e::client> stream (std::move(accepted));

// Read a command with parameters
std::string cmd;
double a, b;
stream >> cmd >> a >> b;  // e.g. "ADD 10.5 20.3"

// Send result back
stream << "RESULT " << (a + b) << std::endl;
```

#### Why is this useful?

* No manual `send()`/`recv()` buffer management
* Works with any code that accepts `std::istream` or `std::ostream`
* Buffered I/O — fewer system calls, better performance
* Familiar API — no new abstractions to learn

See [`examples/tcp_stream.cpp`](examples/tcp_stream.cpp) for complete working examples including a line-by-line echo server and a stream-based calculator.

### 🔧 RAW Socket — send packets with custom IP headers

RAW sockets allow you to build the entire IP packet manually (IP header + any protocol payload)
with full control over every header field — including the source IP address
(useful for traffic forwarding, spoofing, or implementing custom protocols).

```cpp
// Create a RAW socket (requires administrator/root privileges)
udp_socket_t<v4, socket_type_e::client> sock (log_e::debug, udp_type_e::raw);
sock.open("127.0.0.1:9000");

// Build IP + UDP + payload manually, then send
char packet[1500];
int  len = build_raw_packet(packet, sizeof(packet), src_addr, dst_addr, "Hello", 6);
sock.sendto(packet, len, dst_addr);
```

The log shows `raw<...>` prefix so you can distinguish RAW from regular UDP sockets:
```
raw<ip4,client>: [3].open()   [undefined <> 127.0.0.1:9000] success, socket in RAW mode
raw<ip4,client>: [3].sendto() [127.0.0.1:8000 -> 127.0.0.1:9000] sended 34 bytes
```

> **Note:** RAW sockets require administrator privileges on Windows or root/CAP_NET_RAW on Linux.
> Only `sendto()` and `recvfrom()` are available in RAW mode; `send()` and `recv()` are blocked.

See [`examples/raw_socket.cpp`](examples/raw_socket.cpp) for a working send example.

### 📚 More Examples
Check out the [`examples/`](examples) directory for complete working examples:
* [`ip_address.cpp`](examples/ip_address.cpp)     - all IP address manipulation features
* [`udp_socket.cpp`](examples/udp_socket.cpp)     - UDP client-server interaction
* [`tcp_socket.cpp`](examples/tcp_socket.cpp)     - TCP client-server interaction
* [`tcp_stream.cpp`](examples/tcp_stream.cpp)     - TCP iostream interface (<<, >>, getline over network)
* [`raw_socket.cpp`](examples/raw_socket.cpp)     - RAW socket: sending packets with custom IP headers
* [`resolve_host.cpp`](examples/resolve_host.cpp) - resolving host to ipv4/ipv6 address example
* [`http_server.cpp`](examples/http_server.cpp)   - compact multi-page HTTP server
---

## 🤔 Why is this convenient?

Instead of this:
```cpp
// ...
// ifdef and include hell there for cross platform work
// ...
struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_port = htons(8080);
inet_pton(AF_INET, "192.168.1.100", &addr.sin_addr);
// ... and 20 more lines to send a single packet
```

Just write this:
```cpp
// single include and simple create socket object there
sock.open("192.168.1.100:8080");  // done!
sock.send("Hello", 5);
```

---

## 🤔 Why Blocking I/O with Timeouts?

### ✅ Cross-platform simplicity

Async APIs differ significantly between platforms. Blocking sockets allow a single portable implementation without platform-specific code paths.

### ✅ Linear, readable code

No callbacks, no event loops, no complex state machines. Clean code is better than complex async machinery.

### ✅ Threads are cheap, complexity is expensive

Modern systems handle thousands of threads. For most applications, simplicity beats premature optimization.

### ✅ Fits most real-world use cases

* REST clients
* IoT
* Game servers
* Protocol implementations
* Embedded systems

For very high concurrency workloads, the library can still be used with thread pools or higher-level async frameworks.

## 📄 License

MIT License — free for personal and commercial use.

## ❤️ Contributing

Issues, ideas, and pull requests are welcome!

---

**ip-sockets-cpp-lite — sockets that don't scare you.**
