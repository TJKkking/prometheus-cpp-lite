
// ip-sockets-cpp-lite - RAW socket example (send only, requires administrator/root privileges)
//
// This example demonstrates how to send a hand-crafted UDP packet using a RAW socket.
// The sender builds an IP header + UDP header + payload manually and sends it via sendto().
// The receiver is a normal UDP server socket that receives the packet as regular UDP.
//
// Usage: run as administrator (Windows) or root (Linux).
//
//   [raw client]  --( IP + UDP + payload )--> [regular udp server]
//

#include "udp_socket.h"

#include <thread>
#include <chrono>
#include <cstdio>
#include <cstring>

using namespace ipsockets;

static const addr4_t server_addr = "127.0.0.1:9000";
static const addr4_t sender_addr = "127.0.0.1:8000"; // spoofed source address (can be anything for raw)

// ============================================================
// IP + UDP header structures (packed, network byte order)
// ============================================================

#pragma pack(push, 1)
struct ip_header_t {
  uint8_t  ver_ihl;     // version (4) + IHL (5) = 0x45
  uint8_t  tos;         // type of service
  uint16_t total_len;   // total length (IP header + UDP header + payload)
  uint16_t id;          // identification
  uint16_t flags_frag;  // flags + fragment offset
  uint8_t  ttl;         // time to live
  uint8_t  protocol;    // protocol (17 = UDP)
  uint16_t checksum;    // header checksum (kernel fills this when IP_HDRINCL is set)
  ip4_t    src_ip;      // source IP address
  ip4_t    dst_ip;      // destination IP address
};

struct udp_header_t {
  uint16_t src_port;    // source port
  uint16_t dst_port;    // destination port
  uint16_t length;      // UDP header + payload length
  uint16_t checksum;    // UDP checksum (0 = disabled)
};
#pragma pack(pop)

// ============================================================
// build_raw_packet - constructs IP + UDP + payload in buffer
// ============================================================

int build_raw_packet (char* buf, int buf_size, const addr4_t& src, const addr4_t& dst, const char* payload, int payload_len) {

  int ip_hdr_len  = sizeof (ip_header_t);
  int udp_hdr_len = sizeof (udp_header_t);
  int total_len   = ip_hdr_len + udp_hdr_len + payload_len;

  if (total_len > buf_size)
    return -1;

  memset (buf, 0, total_len);

  // fill IP header
  ip_header_t* ip   = (ip_header_t*)buf;
  ip->ver_ihl       = 0x45; // IPv4, IHL = 5 (20 bytes, no options)
  ip->tos           = 0;
  ip->total_len     = orders::htonT<uint16_t> ((uint16_t)total_len);
  ip->id            = orders::htonT<uint16_t> (0x1234);
  ip->ttl           = 64;
  ip->protocol      = IPPROTO_UDP;
  ip->checksum      = 0; // kernel calculates this when IP_HDRINCL is set
  ip->src_ip        = src.ip;
  ip->dst_ip        = dst.ip;

  // fill UDP header
  udp_header_t* udp = (udp_header_t*)(buf + ip_hdr_len);
  udp->src_port     = orders::htonT (src.port);
  udp->dst_port     = orders::htonT (dst.port);
  udp->length       = orders::htonT<uint16_t> ((uint16_t)(udp_hdr_len + payload_len));
  udp->checksum     = 0; // 0 = checksum disabled (valid for UDP)

  // copy payload
  memcpy (buf + ip_hdr_len + udp_hdr_len, payload, payload_len);

  return total_len;
}

// ============================================================
// receiver - regular UDP server (receives normal UDP payload)
// ============================================================

bool shutdown_receiver = false;

void receiver_func () {

  udp_socket_t<v4, socket_type_e::server> sock (log_e::debug);
  if (sock.open (server_addr) != no_error) {
    printf ("receiver: failed to open server socket\n");
    return;
  }

  char    buf[1500];
  addr4_t from;
  while (!shutdown_receiver) {
    int res = sock.recvfrom (buf, sizeof (buf), from);
    if (res > 0) {
      buf[res] = '\0';
      printf ("receiver: got %d bytes from %s: \"%s\"\n", res, from.to_str().c_str(), buf);
    }
    else if (res == error_timeout) {
      // normal timeout, keep waiting
    }
    else
      printf ("receiver: error %d\n", res);
  }
  printf ("receiver: shutdown\n");
}

// ============================================================
// sender - RAW socket client (sends hand-crafted IP+UDP packet)
// ============================================================

void sender_func () {
  udp_socket_t<v4, socket_type_e::client> sock (log_e::debug, udp_type_e::raw);
  if (sock.open (server_addr) != no_error) {
    printf ("sender: failed to open raw socket (need admin/root privileges)\n");
    return;
  }

  std::this_thread::sleep_for (std::chrono::milliseconds (500)); // wait for receiver to start

  const char* messages[] = { "Hello from RAW socket!", "Second RAW packet", "Goodbye!" };

  for (int i = 0; i < 3; i++) {
    char    packet[1500];
    int     payload_len = (int)strlen (messages[i]) + 1; // include null terminator
    int     packet_len  = build_raw_packet (packet, sizeof (packet), sender_addr, server_addr, messages[i], payload_len);
    addr4_t dst         = server_addr;

    if (packet_len > 0) {
      int res = sock.sendto (packet, packet_len, dst);
      if (res > 0) printf ("sender: sent %d bytes (payload: \"%s\")\n", res, messages[i]);
      else         printf ("sender: sendto failed with error %d\n",     res);
    }

    std::this_thread::sleep_for (std::chrono::seconds (1));
  }

  printf ("sender: done\n");
}

// ============================================================
// main
// ============================================================

int main () {

  printf ("=== RAW socket example ===\n");
  printf ("NOTE: requires administrator (Windows) or root (Linux) privileges.\n\n");

  std::thread receiver (receiver_func);
  std::thread sender   (sender_func);

  sender.join ();

  // give receiver time to process last packet
  std::this_thread::sleep_for (std::chrono::seconds (1));
  shutdown_receiver = true;
  receiver.join ();

  printf ("\ndemo app shutdown\n");

  return 0;
}
