
#include "udp_socket.h"
#include <thread>
#include <chrono>

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

using udp_server_t = udp_socket_t<cfg_ip_type, socket_type_e::server>;
using udp_client_t = udp_socket_t<cfg_ip_type, socket_type_e::client>;

// Simple server loop accepting connections.
bool shutdown_server = false;
void server_func () {
  udp_server_t sock (log_e::debug);
  if (sock.open (cfg_server) == ipsockets::no_error) {
    addr_t<cfg_ip_type> client_addr;
    char   buf[1000];
    while (shutdown_server == false) {
      int res = sock.recvfrom (buf, 1000, client_addr);
      if (res >= 0) {  // success
        printf ("server received: %s\n", buf); // print received data
        sock.sendto ("answer\0", 7, client_addr);
      }
      else if (res == ipsockets::error_timeout)
        printf ("server: recv timeout\n");
      else if (res == ipsockets::error_unreachable)
        printf ("server: client unreachable\n");
    }
  }
  printf ("server shutdown\n");
}

// Simple client sending periodic requests.
void client_func () {
  udp_client_t sock (log_e::debug);
  if (sock.open (cfg_client) == ipsockets::no_error) {

    for (size_t i = 0; i < 2; i++) {
      std::this_thread::sleep_for (std::chrono::seconds (1));
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
      }
    }

  }
}

int main () {

  // start server and client in parallel
  std::thread server (server_func);
  std::thread client (client_func);

  // wait for client is finish
  client.join ();

  // wait for server is finish
  shutdown_server = true;
  server.join ();

  printf ("demo app shutdown\n");
  return 0;

}
