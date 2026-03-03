
#include "ip_address.h"

#include <iostream>
#include <string>
#include <unordered_set>

using namespace ipsockets;

#define CHECK(expr, name) \
  do { \
    bool ok = (expr); \
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << '\n'; \
    if (!ok) failures++; \
  } while(0)

int main () {

  int failures = 0;

  std::cout << "========================================\n";
  std::cout << "  IPv4 Address Tests\n";
  std::cout << "========================================\n\n";

  // --- Zero-copy overlay ---

  std::cout << "--- Zero-copy overlay ---\n";

  uint8_t fragment[10] = {0xff, 0xc0, 0xa8, 0x02, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff};
  ip4_t& ipv4_overlay = *((ip4_t*)&fragment[1]);

  CHECK (ipv4_overlay.to_str () == "192.168.2.1",  "overlay on raw memory");
  CHECK (ipv4_overlay[0] == 0xc0,                  "overlay byte[0]");
  CHECK (ipv4_overlay[3] == 0x01,                  "overlay byte[3]");

  // --- Construction from different formats ---

  std::cout << "\n--- Construction (braces) ---\n";

  ip4_t ipv4_00;
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

  CHECK (ipv4_01.to_str () == "0.0.0.0",       "braces {} -> 0.0.0.0");
  CHECK (ipv4_02.to_str () == "192.168.1.2",   "braces from array");
  CHECK (ipv4_03.to_str () == "192.168.1.2",   "braces from uint32 hex");
  CHECK (ipv4_04.to_str () == "192.168.1.2",   "braces from 4 bytes");
  CHECK (ipv4_05.to_str () == "192.168.1.2",   "braces from hex string");
  CHECK (ipv4_06.to_str () == "192.168.1.2",   "braces from decimal string");
  CHECK (ipv4_07.to_str () == "192.168.1.2",   "braces from mixed hex dotted");
  CHECK (ipv4_08.to_str () == "192.168.1.2",   "braces from dotted decimal");
  CHECK (ipv4_09.to_str () == "192.168.0.2",   "braces from 3 octets (missing = 0)");
  CHECK (ipv4_0a.to_str () == "127.0.0.1",     "braces from 2 octets 127.1");

  // all formats produce the same IP
  CHECK (ipv4_02 == ipv4_03, "array == uint32");
  CHECK (ipv4_03 == ipv4_04, "uint32 == 4 bytes");
  CHECK (ipv4_04 == ipv4_05, "4 bytes == hex string");
  CHECK (ipv4_05 == ipv4_06, "hex string == decimal string");
  CHECK (ipv4_06 == ipv4_07, "decimal string == mixed hex dotted");
  CHECK (ipv4_07 == ipv4_08, "mixed hex dotted == dotted decimal");

  std::cout << "\n--- Construction (assignment) ---\n";

  ip4_t ipv4_11 = ip4_t { "127.1" };                                    (void)ipv4_11;
  ip4_t ipv4_12 = std::array<uint8_t, 4> ({ 0xc0, 0xa8, 0x01, 0x02 }); (void)ipv4_12;
  ip4_t ipv4_13 = 0xc0a80102;                                           (void)ipv4_13;
  ip4_t ipv4_14 = { 0xc0, 0xa8, 0x01, 0x02 };                          (void)ipv4_14;
  ip4_t ipv4_15 = "0xc0a80102";                                         (void)ipv4_15;
  ip4_t ipv4_16 = "3232235778";                                         (void)ipv4_16;
  ip4_t ipv4_17 = "0xc0.0xa8.0x01.0x02";                               (void)ipv4_17;
  ip4_t ipv4_18 = "192.168.1.2";                                        (void)ipv4_18;
  ip4_t ipv4_19 = "192.168.2";                                          (void)ipv4_19;
  ip4_t ipv4_1a = "127.1";                                              (void)ipv4_1a;

  CHECK (ipv4_11.to_str () == "127.0.0.1",     "assign from ip4_t");
  CHECK (ipv4_12.to_str () == "192.168.1.2",   "assign from array");
  CHECK (ipv4_13.to_str () == "192.168.1.2",   "assign from uint32");
  CHECK (ipv4_14.to_str () == "192.168.1.2",   "assign from init list");
  CHECK (ipv4_15.to_str () == "192.168.1.2",   "assign from hex string");
  CHECK (ipv4_16.to_str () == "192.168.1.2",   "assign from decimal string");
  CHECK (ipv4_17.to_str () == "192.168.1.2",   "assign from mixed hex dotted");
  CHECK (ipv4_18.to_str () == "192.168.1.2",   "assign from dotted decimal");
  CHECK (ipv4_19.to_str () == "192.168.0.2",   "assign from 3 octets");
  CHECK (ipv4_1a.to_str () == "127.0.0.1",     "assign from 2 octets");

  // --- from_str chaining ---

  std::cout << "\n--- from_str chaining ---\n";

  ip4_t ipv4_25;
  ipv4_25.from_str ("0xc0a80102");
  CHECK (ipv4_25.to_str () == "192.168.1.2",   "from_str hex");

  ip4_t ipv4_26;
  ipv4_26.from_str ("3232235778");
  CHECK (ipv4_26.to_str () == "192.168.1.2",   "from_str decimal");

  ip4_t ipv4_27;
  std::string chained = ipv4_27.from_str ("0xc0.0xa8.0x01.0x02").to_str ();
  CHECK (chained == "192.168.1.2",             "from_str().to_str() chaining");

  // --- Type conversions ---

  std::cout << "\n--- Type conversions ---\n";

  ip4_t ipv4_30 = 0xc0a80102;

  uint32_t               val_u32 = ipv4_30;          
  std::string            val_str = ipv4_30;          
  ip_t<v4>               val_ipt = ipv4_30;          
  std::array<uint8_t, 4> val_arr = ipv4_30;          
  bool                   val_bt  = ipv4_30;          
  bool                   val_bf  = ip4_t("0.0.0.0"); 

  CHECK (val_u32 == 0xc0a80102,             "convert to uint32");
  CHECK (val_str == "192.168.1.2",          "convert to string");
  CHECK (val_ipt == ipv4_30,                "convert to ip_t<v4>");
  CHECK (val_arr[0] == 0xc0,               "convert to array [0]");
  CHECK (val_arr[3] == 0x02,               "convert to array [3]");
  CHECK (val_bt  == true,                   "bool(192.168.1.2) = true");
  CHECK (val_bf  == false,                  "bool(0.0.0.0) = false");

  // --- Bitwise operations ---

  std::cout << "\n--- Bitwise operations ---\n";

  ip4_t result4_and = ip4_t { 192, 168, 2, 1 } & ip4_t { 255, 255, 0, 0 };
  CHECK (result4_and.to_str () == "192.168.0.0",  "bitwise AND");

  ip4_t result4_mask = ip4_t("192.168.2.1");
  result4_mask.set_mask (16);
  CHECK (result4_mask.to_str () == "192.168.0.0", "set_mask(16)");

  ip4_t mask24 ((uint8_t)24);
  CHECK (mask24.to_str () == "255.255.255.0",     "mask from prefix 24");

  ip4_t mask8 ((uint8_t)8);
  CHECK (mask8.to_str () == "255.0.0.0",          "mask from prefix 8");

  ip4_t mask0 ((uint8_t)0);
  CHECK (mask0.to_str () == "0.0.0.0",            "mask from prefix 0");

  ip4_t mask32 ((uint8_t)32);
  CHECK (mask32.to_str () == "255.255.255.255",   "mask from prefix 32");

  // --- Equality ---

  std::cout << "\n--- Equality ---\n";

  CHECK ((ip4_t { 192, 168, 2, 1 } == ip4_t { 192, 168, 2, 1 }) == true,   "equal IPs");
  CHECK ((ip4_t { 192, 168, 2, 1 } == ip4_t { 192, 168, 2, 2 }) == false,  "not equal IPs");

  // --- Rotate ---

  std::cout << "\n--- Rotate ---\n";

  ip4_t ipv4_rot = "1.2.3.4";
  ipv4_rot.rotate ();
  CHECK (ipv4_rot.to_str () == "4.3.2.1",  "rotate 1.2.3.4 -> 4.3.2.1");

  // --- Stream output ---

  std::cout << "\n--- Stream output (visual check) ---\n";
  std::cout << "  " << ip4_t {} << '\n';
  std::cout << "  " << ip4_t { 0xc0a80102 } << '\n';
  std::cout << "  " << ip4_t { "127.1" } << '\n';

  // --- Hash ---

  std::cout << "\n--- Hash ---\n";

  std::unordered_set<ip4_t> set4;
  set4.insert (ip4_t("10.0.0.1"));
  set4.insert (ip4_t("192.168.1.1"));
  set4.insert (ip4_t("10.0.0.1")); // duplicate

  CHECK (set4.size () == 2,                          "hash set size (no duplicates)");
  CHECK (set4.count (ip4_t("10.0.0.1")) == 1,        "hash set find");
  CHECK (set4.count (ip4_t("172.16.0.1")) == 0,      "hash set not find");


  std::cout << "\n========================================\n";
  std::cout << "  IPv6 Address Tests\n";
  std::cout << "========================================\n\n";

  // --- Construction (braces) ---

  std::cout << "--- Construction (braces) ---\n";

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

  CHECK (ipv6_01.to_str () == "::",                                         "braces {} -> ::");
  CHECK (ipv6_02 == ipv6_03,                                                "array == uint16s");
  CHECK (ipv6_03 == ipv6_04,                                                "uint16s == full string");
  CHECK (ipv6_04.to_str () == "1111:2222:3333:4444:5555:6666:7777:8888",   "full address string");
  CHECK (ipv6_05.to_str () == "1111::3333:4444:5555:6666:7777:8888",       ":: compression middle");
  CHECK (ipv6_06.to_str () == "1111::4444:5555:6666:7777:8888",            ":: compression 2 groups");
  CHECK (ipv6_07.to_str () == "1111:2222::4444:5555:6666:7777:8888",       ":: compression after 2222");
  CHECK (ipv6_08.to_str () == "::3333:4444:5555:6666:7777:8888",           ":: at start");
  CHECK (ipv6_09.to_str () == "1111:2222:3333:4444:5555:6666::",           ":: at end");
  CHECK (ipv6_0a.to_str () == "::",                                         ":: only");
  CHECK (ipv6_0b.to_str () == "1111:2222:3333:4444:5555:6666:c0a8:1801",   "embedded ipv4 parsed to hex");
  CHECK (ipv6_0c.to_str () == "::ffff:1.2.3.4",                            "::ipv4 -> ::ffff:ipv4");
  CHECK (ipv6_0d.to_str () == "::ffff:1.2.3.4",                            "bare ipv4 -> ::ffff:ipv4");
  CHECK (ipv6_0e.to_str () == "::ffff:192.168.1.2",                        "from ip4_t -> ::ffff:mapped");
  CHECK (ipv6_0f.to_str () == "::ffff:192.168.1.2",                        "from array<4> -> ::ffff:mapped");

  // --- Construction (assignment) ---

  std::cout << "\n--- Construction (assignment) ---\n";

  ip6_t ipv6_12 = std::array<uint8_t, 16> ({ 0x11, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77, 0x88, 0x88 });
  ip6_t ipv6_14 = "1111:2222:3333:4444:5555:6666:7777:8888";
  ip6_t ipv6_1a = "::";
  ip6_t ipv6_1b = "1111:2222:3333:4444:5555:6666:192.168.24.1";
  ip6_t ipv6_1c = "::1.2.3.4";
  ip6_t ipv6_1d = ip4_t { 0xc0, 0xa8, 0x01, 0x02 };

  CHECK (ipv6_12 == ipv6_14,                       "assign array == assign string");
  CHECK (ipv6_1a.to_str () == "::",                "assign ::");
  CHECK (ipv6_1d.to_str () == "::ffff:192.168.1.2","assign from ip4_t");

  // --- from_str chaining ---

  std::cout << "\n--- from_str chaining ---\n";

  ip6_t ipv6_25;
  ipv6_25.from_str ("1111:2222:3333:4444:5555:6666:7777:8888");
  CHECK (ipv6_25 == ipv6_04,  "from_str full address");

  ip6_t ipv6_26;
  ipv6_26.from_str ("1111:2222:3333:4444:5555:6666:192.168.24.1");
  CHECK (ipv6_26 == ipv6_0b,  "from_str with embedded ipv4");

  ip6_t ipv6_27;
  std::string chained6 = ipv6_27.from_str ("::").to_str ();
  CHECK (chained6 == "::",    "from_str().to_str() chain");

  // --- Type conversions ---

  std::cout << "\n--- Type conversions ---\n";

  ip6_t ipv6_30 = { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 };

  std::string             val6_str = ipv6_30;
  ip_t<v6>                val6_ipt = ipv6_30;
  std::array<uint8_t, 16> val6_arr = ipv6_30;
  bool                    val6_bt  = ipv6_30;
  bool                    val6_bf  = ip6_t("::");
  ip4_t                   val6_ip4 = ipv6_30.get_ip4 ();

  CHECK (val6_str == "1111:2222:3333:4444:5555:6666:7777:8888",  "convert to string");
  CHECK (val6_ipt == ipv6_30,                                     "convert to ip_t<v6>");
  CHECK (val6_arr[0] == 0x11,                                     "convert to array [0]");
  CHECK (val6_bt  == true,                                        "bool(non-zero) = true");
  CHECK (val6_bf  == false,                                       "bool(::) = false");
  CHECK (val6_ip4.to_str () == "119.119.136.136",                 "get_ip4() last 4 bytes");

  // --- is_ip4 ---

  std::cout << "\n--- is_ip4 ---\n";

  CHECK (ip6_t("::ffff:192.168.1.1").is_ip4 () == true,   "::ffff:x.x.x.x is_ip4");
  CHECK (ip6_t("2001:db8::1").is_ip4 () == false,         "2001:db8::1 not is_ip4");
  CHECK (ip6_t("::").is_ip4 () == false,                   ":: not is_ip4");

  // --- to_str variants ---

  std::cout << "\n--- to_str variants ---\n";

  CHECK (ipv6_1a.to_str (true)  == "::",   ":: reduction=true");
  CHECK (ipv6_1a.to_str (false) == "0:0:0:0:0:0:0:0", ":: reduction=false");

  CHECK (ipv6_1c.to_str (true)        == "::ffff:1.2.3.4",   "::1.2.3.4 reduction=true");
  CHECK (ipv6_1c.to_str (true, true)  == "::ffff:1.2.3.4",   "::1.2.3.4 embedded_ipv4=true");

  std::cout << "  ipv6_1b reduction=true:  " << ipv6_1b.to_str (true)  << '\n';
  std::cout << "  ipv6_1b reduction=false: " << ipv6_1b.to_str (false) << '\n';
  std::cout << "  ipv6_1b embedded=true:   " << ipv6_1b.to_str (true, true) << '\n';

  // --- Bitwise operations ---

  std::cout << "\n--- Bitwise operations ---\n";

  ip6_t result6_and = ip6_t { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 } & ip6_t { "ffff::" };
  CHECK (result6_and.to_str () == "1111::",  "bitwise AND with ffff::");

  ip6_t result6_mask = ip6_t("2001:db8::1");
  result6_mask.set_mask (32);
  CHECK (result6_mask.to_str () == "2001:db8::",  "set_mask(32)");

  ip6_t mask6_64 ((uint8_t)64);
  CHECK (mask6_64[7]  == 0xff,  "mask /64 byte[7] = 0xff");
  CHECK (mask6_64[8]  == 0x00,  "mask /64 byte[8] = 0x00");

  // --- Equality ---

  std::cout << "\n--- Equality ---\n";

  CHECK ((ip6_t { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 }
       == ip6_t { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 }) == true,  "equal ipv6");
  CHECK ((ip6_t { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888 }
       == ip6_t { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x7777 }) == false, "not equal ipv6");

  // --- Generic ip_t<> ---

  std::cout << "\n--- Generic ip_t<> ---\n";

  ip_t<v4> gen_v4 { 192, 168, 2, 1 };
  ip_t<v6> gen_v6 { 0x2001, 0x0db8, 0, 0, 0, 0, 0, 1 };

  CHECK (gen_v4.to_str () == "192.168.2.1",     "ip_t<v4>");
  CHECK (gen_v6.to_str () == "2001:db8::1",     "ip_t<v6>");

  // --- Hash ---

  std::cout << "\n--- Hash ---\n";

  std::unordered_set<ip6_t> set6;
  set6.insert (ip6_t("::1"));
  set6.insert (ip6_t("2001:db8::1"));
  set6.insert (ip6_t("::1")); // duplicate

  CHECK (set6.size () == 2,                       "ipv6 hash set size");
  CHECK (set6.count (ip6_t("::1")) == 1,          "ipv6 hash set find");
  CHECK (set6.count (ip6_t("::2")) == 0,          "ipv6 hash set not find");


  std::cout << "\n========================================\n";
  std::cout << "  addr4_t Tests\n";
  std::cout << "========================================\n\n";

  // --- Construction ---

  std::cout << "--- Construction ---\n";

  addr4_t a4_01 = "127.0.0.1:1234";
  addr4_t a4_02 = "255.255.255.255:65535";
  addr4_t a4_03 (ip4_t("10.0.0.1"), 8080);
  addr4_t a4_04;

  CHECK (a4_01.to_str () == "127.0.0.1:1234",          "addr4 from string");
  CHECK (a4_02.to_str () == "255.255.255.255:65535",    "addr4 max values");
  CHECK (a4_03.to_str () == "10.0.0.1:8080",           "addr4 from ip + port");
  CHECK (a4_04.to_str () == "0.0.0.0:0",               "addr4 default");

  CHECK (a4_01.ip == ip4_t("127.0.0.1"),  "addr4 ip field");
  CHECK (a4_01.port == 1234,               "addr4 port field");

  // --- from_str ---

  std::cout << "\n--- from_str ---\n";

  addr4_t a4_fs;
  a4_fs.from_str ("192.168.1.1:9090");
  CHECK (a4_fs.to_str () == "192.168.1.1:9090",  "addr4 from_str");

  // chaining
  std::string a4_chain = addr4_t().from_str ("10.0.0.1:80").to_str ();
  CHECK (a4_chain == "10.0.0.1:80",  "addr4 from_str chain");

  // --- Type conversions ---

  std::cout << "\n--- Type conversions ---\n";

  std::string a4_str = a4_01;
  bool        a4_bt  = a4_01;
  bool        a4_bf  = addr4_t();

  CHECK (a4_str == "127.0.0.1:1234",  "addr4 to string");
  CHECK (a4_bt  == true,               "addr4 bool(valid) = true");
  CHECK (a4_bf  == false,              "addr4 bool(empty) = false");

  // --- Equality ---

  std::cout << "\n--- Equality ---\n";

  CHECK ((addr4_t("192.168.1.2:123") == addr4_t("192.168.1.2:123")) == true,   "addr4 equal");
  CHECK ((addr4_t("192.168.1.2:123") == addr4_t("192.168.2.2:123")) == false,  "addr4 diff ip");
  CHECK ((addr4_t("192.168.1.2:123") == addr4_t("192.168.1.2:124")) == false,  "addr4 diff port");

  // --- Hash ---

  std::cout << "\n--- Hash ---\n";

  std::unordered_set<addr4_t> aset4;
  aset4.insert (addr4_t("10.0.0.1:80"));
  aset4.insert (addr4_t("10.0.0.1:443"));
  aset4.insert (addr4_t("10.0.0.1:80")); // duplicate

  CHECK (aset4.size () == 2,                              "addr4 hash set size");
  CHECK (aset4.count (addr4_t("10.0.0.1:80")) == 1,      "addr4 hash set find");
  CHECK (aset4.count (addr4_t("10.0.0.1:8080")) == 0,    "addr4 hash set not find");

  // --- Generic addr_t<> ---

  std::cout << "\n--- Generic addr_t<> ---\n";

  addr_t<v4> gen_a4 = "10.0.0.1:80";
  CHECK (gen_a4.to_str () == "10.0.0.1:80",  "addr_t<v4>");


  std::cout << "\n========================================\n";
  std::cout << "  addr6_t Tests\n";
  std::cout << "========================================\n\n";

  // --- Construction ---

  std::cout << "--- Construction ---\n";

  addr6_t a6_01 = "[::]:1234";
  addr6_t a6_02 = "[1111:2222:3333:4444:5555:6666:7777:8888]:65535";
  addr6_t a6_03 = "[ffff:2222:3333:4444:5555:6666:255.255.255.255]:65535";
  addr6_t a6_04 (ip6_t("::1"), 8080);
  addr6_t a6_05 = {};

  std::cout << "  a6_01: " << a6_01 << '\n';
  std::cout << "  a6_02: " << a6_02 << '\n';
  std::cout << "  a6_03: " << a6_03 << '\n';

  CHECK (a6_01.port == 1234,   "addr6 port from [::]");
  CHECK (a6_02.port == 65535,  "addr6 port max");
  CHECK (a6_04.to_str () == "[::1]:8080",  "addr6 from ip + port");
  CHECK (a6_05.port == 0,                  "addr6 default port");

  // --- from_str ---

  std::cout << "\n--- from_str ---\n";

  addr6_t a6_fs;
  a6_fs.from_str ("[2001:db8::1]:9090");
  CHECK (a6_fs.to_str () == "[2001:db8::1]:9090",  "addr6 from_str");

  // --- Type conversions ---

  std::cout << "\n--- Type conversions ---\n";

  std::string a6_str = a6_04;
  bool        a6_bt  = a6_04;
  bool        a6_bf  = addr6_t();

  CHECK (a6_str == "[::1]:8080",  "addr6 to string");
  CHECK (a6_bt  == true,          "addr6 bool(valid) = true");
  // Note: addr6_t default has ip=:: (all zeros) and port=0, bool checks (ip && port != 0)
  // ip6_t(::) is false, so bool is false
  CHECK (a6_bf  == false,         "addr6 bool(empty) = false");

  // --- Equality ---

  std::cout << "\n--- Equality ---\n";

  CHECK ((addr6_t("[::192.168.1.2]:123") == addr6_t("[::192.168.1.2]:123")) == true,   "addr6 equal");
  CHECK ((addr6_t("[::192.168.1.2]:123") == addr6_t("[::192.168.2.2]:123")) == false,  "addr6 diff ip");
  CHECK ((addr6_t("[::192.168.1.2]:123") == addr6_t("[::192.168.1.2]:124")) == false,  "addr6 diff port");

  // --- to_str variants ---

  std::cout << "\n--- to_str variants ---\n";

  addr6_t a6_ts (ip6_t("2001:db8::1"), 80);
  std::cout << "  default:      " << a6_ts.to_str ()             << '\n';
  std::cout << "  no reduction: " << a6_ts.to_str (false)        << '\n';
  std::cout << "  embedded v4:  " << a6_ts.to_str (true, true)   << '\n';

  // --- Hash ---

  std::cout << "\n--- Hash ---\n";

  std::unordered_set<addr6_t> aset6;
  aset6.insert (addr6_t("[::1]:80"));
  aset6.insert (addr6_t("[::1]:443"));
  aset6.insert (addr6_t("[::1]:80")); // duplicate

  CHECK (aset6.size () == 2,                              "addr6 hash set size");
  CHECK (aset6.count (addr6_t("[::1]:80")) == 1,          "addr6 hash set find");
  CHECK (aset6.count (addr6_t("[::1]:8080")) == 0,        "addr6 hash set not find");

  // --- Generic addr_t<> ---

  std::cout << "\n--- Generic addr_t<> ---\n";

  addr_t<v6> gen_a6 = "[::1]:80";
  CHECK (gen_a6.to_str () == "[::1]:80",  "addr_t<v6>");


  std::cout << "\n========================================\n";
  if (failures == 0)
    std::cout << "  All tests PASSED!\n";
  else
    std::cout << "  " << failures << " test(s) FAILED!\n";
  std::cout << "========================================\n";

  return failures;

}
