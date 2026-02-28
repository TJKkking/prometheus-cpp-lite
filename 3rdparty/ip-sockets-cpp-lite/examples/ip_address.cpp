
#include "ip_address.h"

using namespace ipsockets;

int main () {

  // IPv4

  uint8_t fragment[10] = {0xff,  0xc0, 0xa8, 0x02, 0x01,  0xff,0xff,0xff,0xff,0xff};
  ip4_t& ipv4ttt = *((ip4_t*)&fragment[1]);
  std::cout << ipv4ttt << '\n';

  ip4_t ipv4_00; (void)ipv4_00;
  ip4_t ipv4_01 {};
  ip4_t ipv4_02 { std::array<uint8_t, 4> ({ 0xc0, 0xa8, 0x01, 0x02 }) };
  ip4_t ipv4_03 { 0xc0a80102 };
  ip4_t ipv4_04 { 0xc0, 0xa8, 0x01, 0x02 };
  ip4_t ipv4_05 { "0xc0a80102" };
  ip4_t ipv4_06 { "3232235778" };
  ip4_t ipv4_07 { "0xc0.0xa8.0x01.0x02" };
  ip4_t ipv4_08 { "192.168.1.2" };
  ip4_t ipv4_09 { "192.168.2" };
  ip4_t ipv4_0a { "127.1" };

  ip4_t ipv4_11 = ip4_t { "127.1" };                                   (void)ipv4_11;
  ip4_t ipv4_12 = std::array<uint8_t, 4> ({ 0xc0, 0xa8, 0x01, 0x02 }); (void)ipv4_12;
  ip4_t ipv4_13 = 0xc0a80102;                                          (void)ipv4_13;
  ip4_t ipv4_14 = { 0xc0, 0xa8, 0x01, 0x02 };                          (void)ipv4_14;
  ip4_t ipv4_15 = "0xc0a80102";                                        (void)ipv4_15;
  ip4_t ipv4_16 = "3232235778";                                        (void)ipv4_16;
  ip4_t ipv4_17 = "0xc0.0xa8.0x01.0x02";                               (void)ipv4_17;
  ip4_t ipv4_18 = "192.168.1.2";                                       (void)ipv4_18;
  ip4_t ipv4_19 = "192.168.2";                                         (void)ipv4_19;
  ip4_t ipv4_1a = "127.1";                                             (void)ipv4_1a;

  ip4_t ipv4_25;
  ip4_t ipv4_26;
  ipv4_25.from_str ("0xc0a80102");
  ipv4_26.from_str ("3232235778");
  ipv4_25.to_str ();
  ipv4_26.to_str ();
  ip4_t ipv4_27;
  ipv4_27.from_str ("0xc0.0xa8.0x01.0x02").to_str ();

  ip4_t                  ipv4_30 = 0xc0a80102;
  uint32_t               ipv4_31 = ipv4_30;          (void)ipv4_31;
  std::string            ipv4_32 = ipv4_30;          (void)ipv4_32;
  ip_t<v4>               ipv4_33 = ipv4_30;          (void)ipv4_33;
  std::array<uint8_t, 4> ipv4_34 = ipv4_30;          (void)ipv4_34;
  bool                   ipv4_35 = ipv4_30;          (void)ipv4_35;
  bool                   ipv4_36 = ip4_t("0.0.0.0"); (void)ipv4_36;

  std::cout << ipv4_00 << '\n';
  std::cout << ip4_t {} << '\n';
  std::cout << ip4_t { std::array<uint8_t, 4> ({ 0xc0, 0xa8, 0x01, 0x02 }) } << '\n';
  std::cout << ip4_t { 0xc0a80102 } << '\n';
  std::cout << ip4_t { 0xc0, 0xa8, 0x01, 0x02 } << '\n';
  std::cout << ip4_t { "0xc0a80102" } << '\n';
  std::cout << ip4_t { "3232235778" } << '\n';
  std::cout << ip4_t { "0xc0.0xa8.0x01.0x02" } << '\n';
  std::cout << ip4_t { "192.168.1.2" } << '\n';
  std::cout << ip4_t { "192.168.2" } << '\n';
  std::cout << ip4_t { "127.1" } << '\n';

  ip4_t result4_1 = ip4_t { 192, 168, 2, 1 }  & ip4_t { 255, 255, 0, 0 }; (void)result4_1;
  bool  result4_2 = ip4_t { 192, 168, 2, 1 } == ip4_t { 192, 168, 2, 1 }; (void)result4_2;
  bool  result4_3 = ip4_t { 192, 168, 2, 1 } == ip4_t { 192, 168, 2, 2 }; (void)result4_3;

  // IPv6

  ip6_t ipv6_00; (void)ipv6_00;
  ip6_t ipv6_01 {};
  ip6_t ipv6_02 { std::array<uint8_t, 16> ({ 0x11, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77, 0x88, 0x88 }) };
  ip6_t ipv6_03 { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 };
  ip6_t ipv6_04 { "1111:2222:3333:4444:5555:6666:7777:8888" };
  ip6_t ipv6_05 { "1111::3333:4444:5555:6666:7777:8888" };
  ip6_t ipv6_06 { "1111::4444:5555:6666:7777:8888" };
  ip6_t ipv6_07 { "1111:2222::4444:5555:6666:7777:8888" };
  ip6_t ipv6_08 { "::3333:4444:5555:6666:7777:8888" };
  ip6_t ipv6_09 { "1111:2222:3333:4444:5555:6666::" };
  ip6_t ipv6_0a { "::" };
  ip6_t ipv6_0b { "1111:2222:3333:4444:5555:6666:192.168.24.1" };
  ip6_t ipv6_0c { "::1.2.3.4" };
  ip6_t ipv6_0d { "1.2.3.4" };
  ip6_t ipv6_0e { ip4_t { 0xc0, 0xa8, 0x01, 0x02 } };
  ip6_t ipv6_0f { std::array<uint8_t, 4> ({ 0xc0, 0xa8, 0x01, 0x02 }) };

  ip6_t ipv6_12 = std::array<uint8_t, 16> ({ 0x11, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77, 0x88, 0x88 }); (void)ipv6_12;
  ip6_t ipv6_13 = { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 };                                                           (void)ipv6_13;
  ip6_t ipv6_14 = "1111:2222:3333:4444:5555:6666:7777:8888";                                                                                    (void)ipv6_14;
  ip6_t ipv6_15 = "1111::3333:4444:5555:6666:7777:8888";                                                                                        (void)ipv6_15;
  ip6_t ipv6_16 = "1111::4444:5555:6666:7777:8888";                                                                                             (void)ipv6_16;
  ip6_t ipv6_17 = "1111:2222::4444:5555:6666:7777:8888";                                                                                        (void)ipv6_17;
  ip6_t ipv6_18 = "::3333:4444:5555:6666:7777:8888";                                                                                            (void)ipv6_18;
  ip6_t ipv6_19 = "1111:2222:3333:4444:5555:6666::";                                                                                            (void)ipv6_19;
  ip6_t ipv6_1a = "::";                                                                                                                         (void)ipv6_1a;
  ip6_t ipv6_1b = "1111:2222:3333:4444:5555:6666:192.168.24.1";                                                                                 (void)ipv6_1b;
  ip6_t ipv6_1c = "::1.2.3.4";                                                                                                                  (void)ipv6_1c;
  ip6_t ipv6_1d = ip4_t { 0xc0, 0xa8, 0x01, 0x02 };                                                                                             (void)ipv6_1d;
  ip6_t ipv6_1e = std::array<uint8_t, 4> ({ 0xc0, 0xa8, 0x01, 0x02 });                                                                          (void)ipv6_1e;

  ip6_t ipv6_25;
  ip6_t ipv6_26;
  ipv6_25.from_str ("1111:2222:3333:4444:5555:6666:7777:8888");
  ipv6_26.from_str ("1111:2222:3333:4444:5555:6666:192.168.24.1");
  ipv6_25.to_str ();
  ipv6_26.to_str ();
  ip6_t ipv6_27;
  ipv6_27.from_str ("::").to_str ();

  ip6_t                   ipv6_30 = { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 };
  std::string             ipv6_32 = ipv6_30;            (void)ipv6_32;
  ip_t<v6>                ipv6_33 = ipv6_30;            (void)ipv6_33;
  std::array<uint8_t, 16> ipv6_34 = ipv6_30;            (void)ipv6_34;
  bool                    ipv6_35 = ipv6_30;            (void)ipv6_35;
  bool                    ipv6_36 = ip6_t("::");        (void)ipv6_36;
  ip4_t                   ipv6_37 = ipv6_30.get_ip4 (); (void)ipv6_37;

  std::cout << ipv6_00 << '\n';
  std::cout << ip6_t {} << '\n';
  std::cout << ip6_t { std::array<uint8_t, 16> ({ 0x11, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77, 0x88, 0x88 }) } << '\n';
  std::cout << ip6_t { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 } << '\n';
  std::cout << ip6_t { "1:22:333:4444:5055:6606:7770:8000" } << '\n';
  std::cout << ip6_t { "1111::3333:4444:5555:6666:7777:8888" } << '\n';
  std::cout << ip6_t { "1111::4444:5555:6666:7777:8888" } << '\n';
  std::cout << ip6_t { "1111:2222::4444:5555:6666:7777:8888" } << '\n';
  std::cout << ip6_t { "::3333:4444:5555:6666:7777:8888" } << '\n';
  std::cout << ip6_t { "1111:2222:3333:4444:5555:6666::" } << '\n';
  std::cout << ip6_t { "::" } << '\n';
  std::cout << ip6_t { "1111:2222:3333:4444:5555:6666:255.255.255.255" } << '\n';
  std::cout << ip6_t { "::192.168.24.1" } << '\n';
  std::cout << ip6_t { "192.168.24.1" } << '\n';
  std::cout << ip6_t { ip4_t { 0xc0, 0xa8, 0x01, 0x02 } } << '\n';
  std::cout << ip6_t { std::array<uint8_t, 4> ({ 0xc0, 0xa8, 0x01, 0x02 }) } << '\n';

  std::cout << ipv6_1a.to_str (true) << '\n';
  std::cout << ipv6_1b.to_str (true) << '\n';
  std::cout << ipv6_1c.to_str (true) << '\n';

  std::cout << ipv6_1a.to_str (false) << '\n';
  std::cout << ipv6_1b.to_str (false) << '\n';
  std::cout << ipv6_1c.to_str (false) << '\n';

  std::cout << ipv6_1a.to_str (true, true) << '\n';
  std::cout << ipv6_1b.to_str (true, true) << '\n';
  std::cout << ipv6_1c.to_str (true, true) << '\n';

  ip_t<v4> ipv4 { 192, 168, 2, 1 };         (void)ipv4;
  ip_t<v6> ipv6 { 1, 2, 3, 4, 5, 6, 7, 8 }; (void)ipv6;

  ip6_t result6_1 = ip6_t { 1, 2, 3, 4, 5, 6, 7, 8 }  & ip6_t { "ffff::" };               (void)result6_1;
  bool  result6_2 = ip6_t { 1, 2, 3, 4, 5, 6, 7, 8 } == ip6_t { 1, 2, 3, 4, 5, 6, 7, 8 }; (void)result6_2;
  bool  result6_3 = ip6_t { 1, 2, 3, 4, 5, 6, 7, 8 } == ip6_t { 1, 2, 3, 4, 5, 6, 7, 7 }; (void)result6_3;

  addr4_t addr4;
  addr6_t addr6;
  
  std::cout << addr4.from_str ("127.0.0.1:1234") << '\n';
  std::cout << addr4.from_str ("255.255.255.255:65535") << '\n';

  bool result_addr4_1 = addr4_t { "192.168.1.2:123" } == addr4_t { "192.168.1.2:123" }; (void)result_addr4_1;
  bool result_addr4_2 = addr4_t { "192.168.1.2:123" } == addr4_t { "192.168.2.2:123" }; (void)result_addr4_2;
  bool result_addr4_3 = addr4_t { "192.168.1.2:123" } == addr4_t { "192.168.1.2:124" }; (void)result_addr4_3;

  std::cout << addr6.from_str ("[::]:1234") << '\n';
  std::cout << addr6.from_str ("[1111:2222:3333:4444:5555:6666:7777:8888]:65535") << '\n';
  std::cout << addr6.from_str ("[ffff:2222:3333:4444:5555:6666:255.255.255.255]:65535") << '\n';

  bool result_addr6_1 = addr6_t { "[::192.168.1.2]:123" } == addr6_t { "[::192.168.1.2]:123" }; (void)result_addr6_1;
  bool result_addr6_2 = addr6_t { "[::192.168.1.2]:123" } == addr6_t { "[::192.168.2.2]:123" }; (void)result_addr6_2;
  bool result_addr6_3 = addr6_t { "[::192.168.1.2]:123" } == addr6_t { "[::192.168.1.2]:124" }; (void)result_addr6_3;

  return 0;

}
