
// my tiny header only crossplatform library for working with ip sockets
#include "tcp_socket.h"

#include <thread>
#include <chrono>
#include <cstdio>

using namespace ipsockets;

#if true
// server and client work on IPv4 mode
static const ip_type_e cfg_ip_type = v4;
static const addr4_t   cfg_server  = "127.0.0.1:2000";
static const addr4_t   cfg_client  = "127.0.0.1:2000";
#else
// server and client work on IPv6 mode
static const ip_type_e cfg_ip_type = v6;
static const addr6_t   cfg_server  = "[::1]:2000";
static const addr6_t   cfg_client  = "[::1]:2000";
#endif

using tcp_server_t = tcp_socket_t<cfg_ip_type, socket_type_e::server>;
using tcp_client_t = tcp_socket_t<cfg_ip_type, socket_type_e::client>;

// Handles a single client connection.
// All accepted connections will be automatically closed when will be closed main server connection
// so this thread with this function also will be finish
void accepted_client_func (tcp_client_t accepted_client) {

  while (true) {
    char buf[1000];
    int res = accepted_client.recv (buf, 1000);

    if (res > 0) { // success
      buf[res] = '\0'; // ensure null termination for safe printing
      printf ("server for accepted connection received: %s\n", buf); // print received data
      std::this_thread::sleep_for (std::chrono::seconds (1)); // simulate some processing delay
      accepted_client.send ("answer\0", 7);
    }
    else if (res == ipsockets::error_timeout)
      printf ("server for accepted connection: recv timeout\n");
    else if (res == ipsockets::error_unreachable)
      printf ("server for accepted connection: client unreachable\n");
    else if (res == ipsockets::error_tcp_closed) {
      printf ("server for accepted connection: client closed connection\n");
      return;
    }
  }
  printf ("shutdown server for accepted connection\n");
}

// Simple server loop accepting connections.
// Each connection is handled in a detached thread.
bool shutdown_server = false;
tcp_server_t server_sock (log_e::debug);
void server_func () {
  if (server_sock.open (cfg_server) == ipsockets::no_error) {
    addr_t<cfg_ip_type> accepted_client_addr;

    while (shutdown_server == false) {

      // accept() waiting new connections until server socket will be closed
      tcp_client_t accepted_client = server_sock.accept (accepted_client_addr);
      if (accepted_client.state == state_e::state_opened) {
        // fire-and-forget thread for this connection
        printf ("server: accept new connection\n");
        std::thread accepted_thread = std::thread (accepted_client_func, std::move (accepted_client));
        accepted_thread.detach();
      }
      else
        printf ("server: accepting new connections timeout\n");

    }
  }
  printf ("server shutdown\n");
}


// Simple client sending periodic requests.
void client_func () {
  tcp_client_t sock (log_e::debug);
  if (sock.open (cfg_client) == ipsockets::no_error) {

    for (size_t i = 0; i < 5; i++) {
      std::this_thread::sleep_for (std::chrono::seconds (4));
      if (sock.send ("hello\0", 6) >= 0) {
        printf ("client send 'hello\\0' success\n");
        char buf[1000];
        int  res = sock.recv (buf, 1000);
        if (res > 0) { // success
          buf[res] = '\0';
          printf ("client received: %s\n", buf);
        }
        else if (res == ipsockets::error_timeout)
          printf ("client: send timeout\n");
        else if (res == ipsockets::error_unreachable)
          printf ("client: server unreachable\n");
        else if (res == ipsockets::error_tcp_closed)
          printf ("client: server closed connection\n");
      }
    }

  }
  printf ("client shutdown\n");
}


int main () {

  // start server and client in parallel
  std::thread server (server_func);
  std::thread client (client_func);

  // wait for client is finish
  client.join ();

  // wait for server is finish
  shutdown_server = true;
  server_sock.close();
  server.join ();

  // wait 1 sec for shutdown accepted connetions
  std::this_thread::sleep_for (std::chrono::seconds (1));

  printf ("demo app shutdown\n");
  return 0;

}
