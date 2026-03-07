
#include "tcp_socket.h"

#include <thread>
#include <chrono>
#include <string>
#include <sstream>
#include <cstdio>

using namespace ipsockets;

#if true
// server and client work on IPv4 mode
static const ip_type_e ip_type = v4;
static const addr4_t   ip_server  = "127.0.0.1:3000";
static const addr4_t   ip_client  = "127.0.0.1:3000";
#else
// server and client work on IPv6 mode
static const ip_type_e ip_type = v6;
static const addr6_t   ip_server  = "[::1]:3000";
static const addr6_t   ip_client  = "[::1]:3000";
#endif

using tcp_server_t        = tcp_socket_t<ip_type, socket_type_e::server>;
using tcp_client_t        = tcp_socket_t<ip_type, socket_type_e::client>;
using tcp_stream_client_t = tcp_stream_t<ip_type, socket_type_e::client>;

log_e socket_log_level = log_e::error;

// ============================================================
// Example 1: Basic << and >> operators
// ============================================================
// Server reads lines, client sends lines using iostream operators.
bool         shutdown_server { false };
tcp_server_t server_sock     { socket_log_level };

void example_1_server () {

  if (server_sock.open (ip_server) != no_error)
    return;

  printf ("[server] listening on %s\n", ip_server.to_str ().c_str ());

  while (!shutdown_server) {

    addr_t<ip_type> client_addr;
    tcp_client_t        accepted = server_sock.accept (client_addr);

    if (accepted.state != state_e::opened)
      continue;

    printf ("[server] client connected: %s\n", client_addr.to_str ().c_str ());

    // Wrap accepted socket into a stream
    tcp_stream_client_t stream (std::move (accepted));

    { // --- Example: read with >> operator ---
      std::string word;
      stream >> word;
      if (stream)
        printf ("[server] received word: '%s'\n", word.c_str ());
    }

    { // --- Example: read with std::getline ---
      std::string line;
      if (std::getline (stream, line))
        printf ("[server] received line: '%s'\n", line.c_str ());
    }

    { // --- Example: read number ---
      int number = 0;
      stream >> number;
      if (stream)
        printf ("[server] received number: %d\n", number);
    }

    { // consume trailing newline after number
      std::string rest;
      std::getline (stream, rest);
    }

    { // --- Example: read multi-word line ---
      std::string line;
      if (std::getline (stream, line))
        printf ("[server] received full line: '%s'\n", line.c_str ());
    }

    // --- Example: send response using << ---
    stream << "RESPONSE OK" << std::endl;
    stream << "value=" << 42 << std::endl;
    stream << "pi=" << 3.14159 << std::endl;

    printf ("[server] response sent, closing connection\n");
    // stream destructor flushes and the socket closes
  }

  printf ("[server] shutdown\n");
}

void example_1_client () {

  // small delay to let server start
  std::this_thread::sleep_for (std::chrono::milliseconds (200));

  tcp_client_t sock (socket_log_level);

  if (sock.open (ip_client) != no_error) {
    printf ("[client] failed to connect\n");
    return;
  }

  // Wrap client socket into a stream
  tcp_stream_client_t stream (std::move (sock));

  printf ("[client] connected to %s\n", stream.remote_address ().to_str ().c_str ());

  // --- Send data using << operator ---
  stream << "Hello";                                            // Send a word (server reads with >>)
  stream << " world_from_stream!" << '\n';                      // Send rest of line (server reads with getline — reads until \n)
  stream << 12345 << '\n';                                      // Send a number
  stream << "This is a multi-word message with spaces" << '\n'; // Send a full line with spaces
  stream << std::flush;                                         // Flush to make sure everything is sent
  printf ("[client] all data sent\n");

  { // --- Read response using >> and getline ---
    std::string line;
    while (std::getline (stream, line))
      printf ("[client] received: '%s'\n", line.c_str ());
  }

  printf ("[client] done\n");
}



// ============================================================
// Example 2: Structured data exchange
// ============================================================
// Shows sending/receiving structured data through streams.

void example_2_server_handler (tcp_client_t accepted) {
  tcp_stream_client_t stream (std::move (accepted));

  // Read a command in format: "CMD arg1 arg2"
  std::string cmd;
  double a = 0, b = 0;
  stream >> cmd >> a >> b;

  if (!stream) {
    printf ("[server2] failed to read command\n");
    return;
  }

  printf ("[server2] command='%s' a=%.2f b=%.2f\n", cmd.c_str (), a, b);

  // Compute and send result
  double result = 0;
  if      (cmd == "ADD") result = a + b;
  else if (cmd == "MUL") result = a * b;
  else if (cmd == "SUB") result = a - b;
  else {
    stream << "ERROR unknown command" << std::endl;
    return;
  }

  stream << "RESULT " << result << std::endl;
  printf ("[server2] sent result: %.2f\n", result);
}

void example_2_server () {

  tcp_server_t server (socket_log_level);
  if (server.open ({ip_client.ip, 3001}) != no_error)
    return;

  printf ("[server2] listening for calculator requests\n");

  for (int i = 0; i < 3; i++) {
    addr_t<ip_type> client_addr;
    tcp_client_t accepted = server.accept (client_addr);
    if (accepted.state == state_e::opened)
      example_2_server_handler (std::move (accepted));
  }

  printf ("[server2] shutdown\n");
}

void example_2_client () {

  std::this_thread::sleep_for (std::chrono::milliseconds (200));

  // Send three calculator requests
  struct { const char* cmd; double a; double b; } requests[] = {
    { "ADD",  10.5, 20.3 },
    { "MUL",   3.0,  7.0 },
    { "SUB", 100.0, 42.0 }
  };

  for (int i = 0; i < 3; i++) {
    tcp_client_t sock (socket_log_level);
    if (sock.open ({ip_client.ip, 3001}) != no_error) {
      printf ("[client2] failed to connect\n");
      continue;
    }

    tcp_stream_client_t stream (std::move (sock));

    // Send: "CMD a b\n"
    stream << requests[i].cmd << ' ' << requests[i].a << ' ' << requests[i].b << std::endl;

    // Read response line
    std::string line;
    if (std::getline (stream, line))
      printf ("[client2] %s %.1f %.1f -> %s\n", requests[i].cmd, requests[i].a, requests[i].b, line.c_str ());
  }

  printf ("[client2] done\n");
}




// ============================================================
// Example 3: Line-by-line chat
// ============================================================
// Server echoes every line back in uppercase.

void example_3_server_handler (tcp_client_t accepted) {
  tcp_stream_client_t stream (std::move (accepted));

  std::string line;
  while (std::getline (stream, line)) {
    printf ("[echo-server] received: '%s'\n", line.c_str ());

    if (line == "QUIT")
      break;

    // Convert to uppercase and echo back
    for (char& c : line)
      if ('a' <= c && c <= 'z')
        c = c - 'a' + 'A';

    stream << line << '\n';
  }

  stream << std::flush;
  printf ("[echo-server] client disconnected\n");
}

void example_3_server () {

  tcp_server_t server (socket_log_level);
  if (server.open ({ip_client.ip, 3002}) != no_error)
    return;

  printf ("[echo-server] listening\n");

  addr_t<ip_type> client_addr;
  tcp_client_t        accepted = server.accept (client_addr);
  if (accepted.state == state_e::opened)
    example_3_server_handler (std::move (accepted));

  printf ("[echo-server] shutdown\n");
}

void example_3_client () {

  std::this_thread::sleep_for (std::chrono::milliseconds (200));

  tcp_client_t sock (socket_log_level);
  if (sock.open ({ip_client.ip, 3002}) != no_error)
    return;

  tcp_stream_client_t stream (std::move (sock));

  // Send several lines and read echoed responses
  const char* messages[] = { "hello world", "ip-sockets-cpp-lite is great", "testing 123" };

  for (const char* msg : messages) {
    stream << msg << '\n';
    stream << std::flush; // ensure data is sent before reading response

    std::string response;
    if (std::getline (stream, response))
      printf ("[echo-client] sent='%s' received='%s'\n", msg, response.c_str ());
  }

  // Send quit command
  stream << "QUIT" << std::endl;

  printf ("[echo-client] done\n");
}


// ============================================================
// Main — run all examples sequentially
// ============================================================

int main () {

  printf ("========================================\n");
  printf ("  Example 1: Basic iostream operators\n");
  printf ("========================================\n\n");

  {
    std::thread server (example_1_server);
    std::thread client (example_1_client);

    client.join ();

    shutdown_server = true;
    server_sock.close ();
    server.join ();
  }

  std::this_thread::sleep_for (std::chrono::milliseconds (500));

  printf ("\n========================================\n");
  printf ("  Example 2: Calculator (structured)\n");
  printf ("========================================\n\n");

  {
    std::thread server (example_2_server);
    std::thread client (example_2_client);

    client.join ();
    server.join ();
  }

  std::this_thread::sleep_for (std::chrono::milliseconds (500));

  printf ("\n========================================\n");
  printf ("  Example 3: Echo server (line-by-line)\n");
  printf ("========================================\n\n");

  {
    std::thread server (example_3_server);
    std::thread client (example_3_client);

    client.join ();
    server.join ();
  }

  printf ("\n========================================\n");
  printf ("  All examples completed!\n");
  printf ("========================================\n");

  return 0;
}