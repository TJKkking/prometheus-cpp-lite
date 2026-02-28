
#include "udp_socket.h"

using namespace ipsockets;

int main () {

  using udp_client_v4_t = udp_socket_t<v4, socket_type_e::client>;
  using udp_client_v6_t = udp_socket_t<v6, socket_type_e::client>;

  // resolve hostname to IPv4

  std::string host1 = "localhost";
  ip4_t host_ip1 = udp_client_v4_t::resolve (host1);
  printf ("host: %s, resolved ip: %s\n\n", host1.c_str(), host_ip1.to_str().c_str());

  std::string host2 = "google.com";
  ip4_t host_ip2 = udp_client_v4_t::resolve (host2, nullptr, log_e::info);
  printf ("host: %s, resolved ip: %s\n\n", host2.c_str(), host_ip2.to_str().c_str());

  std::string host3 = "some_unreachable_host";
  bool resolve_result3;
  ip4_t host_ip3 = udp_client_v4_t::resolve (host3, &resolve_result3, log_e::info);
  printf ("host: %s, resolved result: %s, resolved ip: %s\n\n", host3.c_str(), (resolve_result3) ? "true" : "false", host_ip3.to_str().c_str());

  // resolve hostname to IPv6

  std::string host4 = "localhost";
  ip6_t host_ip4 = udp_client_v6_t::resolve (host4);
  printf ("host: %s, resolved ip: %s\n\n", host4.c_str(), host_ip4.to_str().c_str());

  std::string host5 = "google.com";
  ip6_t host_ip5 = udp_client_v6_t::resolve (host5, nullptr, log_e::info);
  printf ("host: %s, resolved ip: %s\n\n", host5.c_str(), host_ip5.to_str().c_str());

  std::string host6 = "some_unreachable_host";
  bool resolve_result6;
  ip6_t host_ip6 = udp_client_v6_t::resolve (host6, &resolve_result6, log_e::info);
  printf ("host: %s, resolved result: %s, resolved ip: %s\n\n", host6.c_str(), (resolve_result6) ? "true" : "false", host_ip6.to_str().c_str());

}
