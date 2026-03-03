
#include "ip_address.h"

#include <iostream>
#include <string>
#include <cassert>
#include <unordered_set>

using namespace ipsockets;

// helper to print test results
#define CHECK(expr, name) \
  do { \
    bool check_result_ = (expr); \
    std::cout << (check_result_ ? "[PASS] " : "[FAIL] ") << name << '\n'; \
    if (!check_result_) failures++; \
  } while(0)

int main () {

  int failures = 0;

  std::cout << "========================================\n";
  std::cout << "  IPv4 Prefix Tests\n";
  std::cout << "========================================\n\n";

  // --- Construction from CIDR string ---

  std::cout << "--- Construction from CIDR string ---\n";

  prefix4_t p4_01 = "192.168.1.0/24";
  prefix4_t p4_02 = "10.0.0.0/8";
  prefix4_t p4_03 = "172.16.0.0/12";
  prefix4_t p4_04 = "0.0.0.0/0";
  prefix4_t p4_05 = "192.168.1.100/32";

  std::cout << "p4_01: " << p4_01 << '\n';
  std::cout << "p4_02: " << p4_02 << '\n';
  std::cout << "p4_03: " << p4_03 << '\n';
  std::cout << "p4_04: " << p4_04 << '\n';
  std::cout << "p4_05: " << p4_05 << '\n';

  CHECK (p4_01.to_str () == "192.168.1.0/24",    "CIDR parse 192.168.1.0/24");
  CHECK (p4_02.to_str () == "10.0.0.0/8",        "CIDR parse 10.0.0.0/8");
  CHECK (p4_03.to_str () == "172.16.0.0/12",     "CIDR parse 172.16.0.0/12");
  CHECK (p4_04.to_str () == "0.0.0.0/0",         "CIDR parse 0.0.0.0/0");
  CHECK (p4_05.to_str () == "192.168.1.100/32",  "CIDR parse host /32");
  CHECK (p4_01.length == 24,                      "length == 24");
  CHECK (p4_02.length == 8,                       "length == 8");
  CHECK (p4_04.length == 0,                       "length == 0 (default route)");

  // --- Construction from IP + length (auto-masking) ---

  std::cout << "\n--- Construction from IP + length (auto-masking) ---\n";

  prefix4_t p4_06 (ip4_t("192.168.1.123"), 24);
  prefix4_t p4_07 (ip4_t("10.20.30.40"),   16);
  prefix4_t p4_08 (ip4_t("172.16.15.255"), 20);
  prefix4_t p4_09 (ip4_t("255.255.255.255"), 0);

  std::cout << "p4_06: " << p4_06 << '\n';
  std::cout << "p4_07: " << p4_07 << '\n';
  std::cout << "p4_08: " << p4_08 << '\n';
  std::cout << "p4_09: " << p4_09 << '\n';

  CHECK (p4_06.to_str () == "192.168.1.0/24",  "auto-mask 192.168.1.123/24 -> .0/24");
  CHECK (p4_07.to_str () == "10.20.0.0/16",    "auto-mask 10.20.30.40/16 -> .0.0/16");
  CHECK (p4_08.to_str () == "172.16.0.0/20",   "auto-mask 172.16.15.255/20 -> 172.16.0.0/20");
  CHECK (p4_09.to_str () == "0.0.0.0/0",       "auto-mask 255.255.255.255/0 -> 0.0.0.0/0");

  // --- Construction from IP only (host prefix) ---

  std::cout << "\n--- Construction from IP only (host prefix /32) ---\n";

  prefix4_t p4_10 (ip4_t("192.168.1.1"));
  prefix4_t p4_11 (ip4_t("0.0.0.0"));

  std::cout << "p4_10: " << p4_10 << '\n';
  std::cout << "p4_11: " << p4_11 << '\n';

  CHECK (p4_10.to_str () == "192.168.1.1/32", "host prefix 192.168.1.1/32");
  CHECK (p4_10.length == 32,                   "host prefix length == 32");
  CHECK (p4_11.to_str () == "0.0.0.0/32",     "host prefix 0.0.0.0/32");

  // --- Default construction ---

  std::cout << "\n--- Default construction ---\n";

  prefix4_t p4_def;
  std::cout << "p4_def: " << p4_def << '\n';

  CHECK (p4_def.length == 0,                "default length == 0");
  CHECK (p4_def.ip == ip4_t("0.0.0.0"),     "default ip == 0.0.0.0");
  CHECK (p4_def.to_str () == "0.0.0.0/0",   "default to_str");

  // --- from_str with success flag ---

  std::cout << "\n--- from_str with success flag ---\n";

  bool ok = false;

  prefix4_t p4_fs1;
  p4_fs1.from_str ("172.16.0.0/12", &ok);
  CHECK (ok == true,                            "from_str 172.16.0.0/12 success");
  CHECK (p4_fs1.to_str () == "172.16.0.0/12",  "from_str 172.16.0.0/12 value");

  prefix4_t p4_fs2;
  p4_fs2.from_str ("192.168.1.0/24", &ok);
  CHECK (ok == true,                            "from_str 192.168.1.0/24 success");

  // no slash — host prefix
  prefix4_t p4_fs3;
  p4_fs3.from_str ("10.0.0.1", &ok);
  CHECK (ok == true,                            "from_str no slash success");
  CHECK (p4_fs3.to_str () == "10.0.0.1/32",    "from_str no slash -> /32");

  // from std::string
  prefix4_t p4_fs4;
  p4_fs4.from_str (std::string("10.0.0.0/8"), &ok);
  CHECK (ok == true,                            "from_str(string) success");
  CHECK (p4_fs4.to_str () == "10.0.0.0/8",     "from_str(string) value");

  // --- Parse errors ---

  std::cout << "\n--- Parse errors ---\n";

  prefix4_t p4_err1;
  p4_err1.from_str ("not_an_ip/24", &ok);
  CHECK (ok == false,       "error: invalid IP");
  CHECK (p4_err1.length == 0, "error: reset to empty");

  prefix4_t p4_err2;
  p4_err2.from_str ("192.168.1.0/33", &ok);
  CHECK (ok == false,       "error: prefix > 32");

  prefix4_t p4_err3;
  p4_err3.from_str ("192.168.1.0/", &ok);
  CHECK (ok == false,       "error: slash without number");

  prefix4_t p4_err4;
  p4_err4.from_str ("192.168.1.0/abc", &ok);
  CHECK (ok == false,       "error: non-numeric prefix");

  prefix4_t p4_err5;
  p4_err5.from_str ("", &ok);
  CHECK (ok == false,       "error: empty string");

  // auto-masking on CIDR string with host bits set
  prefix4_t p4_mask1 = "192.168.1.255/24";
  CHECK (p4_mask1.to_str () == "192.168.1.0/24",  "CIDR auto-mask host bits");

  prefix4_t p4_mask2 = "10.20.30.40/8";
  CHECK (p4_mask2.to_str () == "10.0.0.0/8",      "CIDR auto-mask /8");

  // --- network() ---

  std::cout << "\n--- network() ---\n";

  CHECK (p4_01.network () == ip4_t("192.168.1.0"),  "network 192.168.1.0/24");
  CHECK (p4_02.network () == ip4_t("10.0.0.0"),     "network 10.0.0.0/8");
  CHECK (p4_05.network () == ip4_t("192.168.1.100"),"network host /32");
  CHECK (p4_04.network () == ip4_t("0.0.0.0"),      "network default /0");

  // --- contains(ip) ---

  std::cout << "\n--- contains(ip) ---\n";

  CHECK (p4_01.contains (ip4_t("192.168.1.0"))   == true,  "/24 contains .0");
  CHECK (p4_01.contains (ip4_t("192.168.1.5"))   == true,  "/24 contains .5");
  CHECK (p4_01.contains (ip4_t("192.168.1.255")) == true,  "/24 contains .255");
  CHECK (p4_01.contains (ip4_t("192.168.2.0"))   == false, "/24 not contains .2.0");
  CHECK (p4_01.contains (ip4_t("192.168.0.255")) == false, "/24 not contains .0.255");

  CHECK (p4_02.contains (ip4_t("10.0.0.0"))       == true,  "/8 contains 10.0.0.0");
  CHECK (p4_02.contains (ip4_t("10.255.255.255")) == true,  "/8 contains 10.255.255.255");
  CHECK (p4_02.contains (ip4_t("11.0.0.0"))       == false, "/8 not contains 11.0.0.0");

  CHECK (p4_04.contains (ip4_t("0.0.0.0"))         == true, "/0 contains 0.0.0.0");
  CHECK (p4_04.contains (ip4_t("255.255.255.255")) == true, "/0 contains 255.255.255.255");
  CHECK (p4_04.contains (ip4_t("1.2.3.4"))         == true, "/0 contains any");

  CHECK (p4_05.contains (ip4_t("192.168.1.100")) == true,  "/32 contains exact");
  CHECK (p4_05.contains (ip4_t("192.168.1.101")) == false, "/32 not contains other");

  // --- contains(prefix) ---

  std::cout << "\n--- contains(prefix) ---\n";

  prefix4_t p4_sub1 = "192.168.1.128/25";
  prefix4_t p4_sub2 = "192.168.1.0/25";
  prefix4_t p4_sub3 = "192.168.0.0/16";

  CHECK (p4_01.contains (p4_sub1)  == true,  "/24 contains /25 (high half)");
  CHECK (p4_01.contains (p4_sub2)  == true,  "/24 contains /25 (low half)");
  CHECK (p4_sub1.contains (p4_01)  == false, "/25 not contains /24");
  CHECK (p4_01.contains (p4_sub3)  == false, "/24 not contains /16");
  CHECK (p4_sub3.contains (p4_01)  == true,  "/16 contains /24");
  CHECK (p4_04.contains (p4_01)    == true,  "/0 contains anything");
  CHECK (p4_01.contains (p4_04)    == false, "/24 not contains /0");

  // self containment
  CHECK (p4_01.contains (p4_01)    == true,  "prefix contains itself");

  // --- Comparison operators ---

  std::cout << "\n--- Comparison operators ---\n";

  CHECK ((prefix4_t("10.0.0.0/8")  == prefix4_t("10.0.0.0/8"))  == true,  "equal prefixes");
  CHECK ((prefix4_t("10.0.0.0/8")  == prefix4_t("10.0.0.0/16")) == false, "different length");
  CHECK ((prefix4_t("10.0.0.0/8")  == prefix4_t("11.0.0.0/8"))  == false, "different ip");
  CHECK ((prefix4_t("10.0.0.0/8")  != prefix4_t("11.0.0.0/8"))  == true,  "not equal");
  CHECK ((prefix4_t("10.0.0.0/8")  != prefix4_t("10.0.0.0/8"))  == false, "not not-equal");
  CHECK ((prefix4_t("10.0.0.0/8")  <  prefix4_t("10.0.0.0/16")) == true,  "less by length");
  CHECK ((prefix4_t("10.0.0.0/16") <  prefix4_t("10.0.0.0/8"))  == false, "not less by length");
  CHECK ((prefix4_t("10.0.0.0/8")  <  prefix4_t("11.0.0.0/8"))  == true,  "less by ip");

  // --- bool conversion ---

  std::cout << "\n--- bool conversion ---\n";

  CHECK ((bool)p4_01  == true,  "bool /24 = true");
  CHECK ((bool)p4_05  == true,  "bool /32 = true");
  CHECK ((bool)p4_def == false, "bool empty = false");
  // Note: /0 has length=0, so bool is false — this is by design (default route is "empty" prefix)
  CHECK ((bool)p4_04  == false, "bool /0 = false (by design)");

  // --- String conversion ---

  std::cout << "\n--- String conversion ---\n";

  std::string s4_01 = p4_01;
  std::string s4_02 = p4_02.to_str ();
  CHECK (s4_01 == "192.168.1.0/24", "implicit string conversion");
  CHECK (s4_02 == "10.0.0.0/8",     "to_str()");

  // --- Bit-level operations ---

  std::cout << "\n--- Bit-level operations ---\n";

  // Build 192 = 0b11000000 bit by bit
  prefix4_t p4_bits;
  p4_bits.push_back_bit (1); // bit 0: 1
  p4_bits.push_back_bit (1); // bit 1: 1
  p4_bits.push_back_bit (0); // bit 2: 0
  p4_bits.push_back_bit (0); // bit 3: 0
  p4_bits.push_back_bit (0); // bit 4: 0
  p4_bits.push_back_bit (0); // bit 5: 0
  p4_bits.push_back_bit (0); // bit 6: 0
  p4_bits.push_back_bit (0); // bit 7: 0

  std::cout << "p4_bits: " << p4_bits << '\n';

  CHECK (p4_bits.length == 8,                       "push_back_bit length");
  CHECK (p4_bits.ip == ip4_t("192.0.0.0"),          "push_back_bit value = 192.0.0.0");
  CHECK (p4_bits.to_str () == "192.0.0.0/8",        "push_back_bit to_str");

  // get_bit
  CHECK (p4_bits.get_bit (0) == true,   "get_bit 0 = 1");
  CHECK (p4_bits.get_bit (1) == true,   "get_bit 1 = 1");
  CHECK (p4_bits.get_bit (2) == false,  "get_bit 2 = 0");
  CHECK (p4_bits.get_bit (3) == false,  "get_bit 3 = 0");
  CHECK (p4_bits.get_bit (7) == false,  "get_bit 7 = 0");

  // set_bit
  prefix4_t p4_setbit (ip4_t("192.168.1.0"), 24);
  p4_setbit.set_bit (23, true); // set last bit of third byte -> 192.168.1.0 -> 192.168.1.0 (bit 23 is already 0, set to 1)
  // byte 2 = 0x01, so ip becomes 192.168.1.0
  // wait: byte[2] = 1 (0x01), bit 23 = byte[2] bit 7 = LSB of byte 2
  // 192.168.1.0/24 -> set bit 23 (last bit of prefix) = byte[2], bit (23 & 7) = 7 -> 0x80 >> 7 = 0x01
  // byte[2] was 1, |= 0x01 -> still 1
  // Let's use a clearer test:
  prefix4_t p4_sb (ip4_t("192.168.0.0"), 24);
  std::cout << "before set_bit: " << p4_sb << '\n';
  p4_sb.set_bit (16, true);  // bit 16 = byte[2] bit 0 = MSB of byte 2 = 0x80 -> 192.168.128.0
  std::cout << "after set_bit(16,1): " << p4_sb << '\n';
  CHECK (p4_sb.ip == ip4_t("192.168.128.0"),  "set_bit(16, true) -> 192.168.128.0");
  CHECK (p4_sb.get_bit (16) == true,           "get_bit(16) after set");

  p4_sb.set_bit (16, false);
  std::cout << "after set_bit(16,0): " << p4_sb << '\n';
  CHECK (p4_sb.ip == ip4_t("192.168.0.0"),    "set_bit(16, false) -> 192.168.0.0");
  CHECK (p4_sb.get_bit (16) == false,          "get_bit(16) after clear");

  // operator<< (creates copy with appended bit)
  prefix4_t p4_shifted = prefix4_t() << 1 << 0 << 1;  // 101 = 0xa0 = 160
  std::cout << "p4_shifted: " << p4_shifted << '\n';
  CHECK (p4_shifted.length == 3,                   "operator<< length");
  CHECK (p4_shifted.ip[0] == 0xa0,                 "operator<< value 0xa0");
  CHECK (p4_shifted.get_bit (0) == true,           "operator<< bit 0");
  CHECK (p4_shifted.get_bit (1) == false,          "operator<< bit 1");
  CHECK (p4_shifted.get_bit (2) == true,           "operator<< bit 2");

  // chain more bits: 10100000.10000000 = 160.128.0.0/10
  prefix4_t p4_chain = p4_shifted << 0 << 0 << 0 << 0 << 0 << 1 << 0;
  std::cout << "p4_chain: " << p4_chain << '\n';
  CHECK (p4_chain.length == 10,                    "chain length 10");
  CHECK (p4_chain.ip == ip4_t("160.128.0.0"),      "chain value");

  // --- Raw overlay ---

  std::cout << "\n--- Raw overlay ---\n";

  uint8_t raw_buf[] = { 24, 192, 168, 1 };
  const ip_prefix_raw_t& raw_ref = *reinterpret_cast<const ip_prefix_raw_t*>(raw_buf);

  CHECK (raw_ref.length == 24,       "raw length");
  CHECK (raw_ref.value_size () == 3, "raw value_size");
  CHECK (raw_ref.size () == 4,       "raw size");
  CHECK (raw_ref.bytes[0] == 192,    "raw bytes[0]");
  CHECK (raw_ref.bytes[1] == 168,    "raw bytes[1]");
  CHECK (raw_ref.bytes[2] == 1,      "raw bytes[2]");
  CHECK (raw_ref.get_bit (0) == true,  "raw get_bit 0 (MSB of 192)");
  CHECK (raw_ref.get_bit (1) == true,  "raw get_bit 1");
  CHECK (raw_ref.get_bit (2) == false, "raw get_bit 2");

  prefix4_t p4_from_raw (raw_ref);
  std::cout << "from raw: " << p4_from_raw << '\n';
  CHECK (p4_from_raw.to_str () == "192.168.1.0/24", "prefix from raw");

  // convert prefix back to raw
  const ip_prefix_raw_t& back_raw = (const ip_prefix_raw_t&)p4_01;
  CHECK (back_raw.length == 24,     "prefix to raw length");
  CHECK (back_raw.bytes[0] == 192,  "prefix to raw bytes[0]");
  CHECK (back_raw.bytes[1] == 168,  "prefix to raw bytes[1]");
  CHECK (back_raw.bytes[2] == 1,    "prefix to raw bytes[2]");

  // --- Hash (unordered_set) ---

  std::cout << "\n--- Hash ---\n";

  std::unordered_set<prefix4_t> set4;
  set4.insert (prefix4_t("10.0.0.0/8"));
  set4.insert (prefix4_t("192.168.1.0/24"));
  set4.insert (prefix4_t("10.0.0.0/8")); // duplicate

  CHECK (set4.size () == 2,                                    "hash set size (no duplicates)");
  CHECK (set4.count (prefix4_t("10.0.0.0/8")) == 1,           "hash set find /8");
  CHECK (set4.count (prefix4_t("192.168.1.0/24")) == 1,       "hash set find /24");
  CHECK (set4.count (prefix4_t("172.16.0.0/12")) == 0,        "hash set not find /12");


  std::cout << "\n========================================\n";
  std::cout << "  IPv6 Prefix Tests\n";
  std::cout << "========================================\n\n";

  // --- Construction from CIDR string ---

  std::cout << "--- Construction from CIDR string ---\n";

  prefix6_t p6_01 = "2001:db8::/32";
  prefix6_t p6_02 = "fe80::/10";
  prefix6_t p6_03 = "::1/128";
  prefix6_t p6_04 = "::/0";
  prefix6_t p6_05 = "2001:db8:abcd:1234::/64";

  std::cout << "p6_01: " << p6_01 << '\n';
  std::cout << "p6_02: " << p6_02 << '\n';
  std::cout << "p6_03: " << p6_03 << '\n';
  std::cout << "p6_04: " << p6_04 << '\n';
  std::cout << "p6_05: " << p6_05 << '\n';

  CHECK (p6_01.length == 32,   "ipv6 /32 length");
  CHECK (p6_02.length == 10,   "ipv6 /10 length");
  CHECK (p6_03.length == 128,  "ipv6 /128 length");
  CHECK (p6_04.length == 0,    "ipv6 /0 length");
  CHECK (p6_05.length == 64,   "ipv6 /64 length");

  // --- Auto-masking ---

  std::cout << "\n--- Auto-masking ---\n";

  prefix6_t p6_mask1 = "2001:db8:ffff:ffff:ffff:ffff:ffff:ffff/32";
  std::cout << "p6_mask1: " << p6_mask1 << '\n';
  CHECK (p6_mask1.ip == ip6_t("2001:db8::"),   "ipv6 auto-mask /32");

  prefix6_t p6_mask2 (ip6_t("fe80::1234:5678"), 10);
  std::cout << "p6_mask2: " << p6_mask2 << '\n';
  CHECK (p6_mask2.ip == ip6_t("fe80::"),        "ipv6 auto-mask /10");

  prefix6_t p6_mask3 (ip6_t("2001:db8:abcd:1234:ffff:ffff:ffff:ffff"), 64);
  std::cout << "p6_mask3: " << p6_mask3 << '\n';
  CHECK (p6_mask3.ip == ip6_t("2001:db8:abcd:1234::"), "ipv6 auto-mask /64");

  // --- Host prefix ---

  std::cout << "\n--- Host prefix ---\n";

  prefix6_t p6_host (ip6_t("::1"));
  std::cout << "p6_host: " << p6_host << '\n';
  CHECK (p6_host.length == 128,                "ipv6 host prefix length");
  CHECK (p6_host.ip == ip6_t("::1"),           "ipv6 host prefix ip");

  // --- network() ---

  std::cout << "\n--- network() ---\n";

  CHECK (p6_01.network () == ip6_t("2001:db8::"),              "ipv6 network /32");
  CHECK (p6_05.network () == ip6_t("2001:db8:abcd:1234::"),   "ipv6 network /64");
  CHECK (p6_03.network () == ip6_t("::1"),                     "ipv6 network /128");

  // --- contains(ip) ---

  std::cout << "\n--- contains(ip) ---\n";

  CHECK (p6_01.contains (ip6_t("2001:db8::1"))       == true,  "/32 contains 2001:db8::1");
  CHECK (p6_01.contains (ip6_t("2001:db8:1::1"))     == true,  "/32 contains 2001:db8:1::1");
  CHECK (p6_01.contains (ip6_t("2001:db9::1"))       == false, "/32 not contains 2001:db9::1");

  CHECK (p6_02.contains (ip6_t("fe80::1"))            == true,  "/10 contains fe80::1");
  CHECK (p6_02.contains (ip6_t("fe80::ffff"))         == true,  "/10 contains fe80::ffff");
  CHECK (p6_02.contains (ip6_t("febf:ffff::1"))       == true,  "/10 contains febf:ffff::1");
  CHECK (p6_02.contains (ip6_t("fec0::1"))            == false, "/10 not contains fec0::1");

  CHECK (p6_04.contains (ip6_t("::1"))                == true,  "/0 contains ::1");
  CHECK (p6_04.contains (ip6_t("ffff::ffff"))         == true,  "/0 contains any");

  CHECK (p6_03.contains (ip6_t("::1"))                == true,  "/128 contains exact");
  CHECK (p6_03.contains (ip6_t("::2"))                == false, "/128 not contains other");

  // --- contains(prefix) ---

  std::cout << "\n--- contains(prefix) ---\n";

  prefix6_t p6_sub1 = "2001:db8:1::/48";
  prefix6_t p6_sub2 = "2001:db8:abcd:1234::/64";

  CHECK (p6_01.contains (p6_sub1)  == true,   "/32 contains /48");
  CHECK (p6_01.contains (p6_sub2)  == true,   "/32 contains /64");
  CHECK (p6_sub1.contains (p6_01)  == false,  "/48 not contains /32");
  CHECK (p6_04.contains (p6_01)    == true,   "/0 contains everything");
  CHECK (p6_01.contains (p6_04)    == false,  "/32 not contains /0");
  CHECK (p6_01.contains (p6_01)    == true,   "self containment");

  // --- Comparison ---

  std::cout << "\n--- Comparison ---\n";

  CHECK ((prefix6_t("2001:db8::/32") == prefix6_t("2001:db8::/32"))  == true,  "ipv6 equal");
  CHECK ((prefix6_t("2001:db8::/32") == prefix6_t("2001:db8::/48"))  == false, "ipv6 diff length");
  CHECK ((prefix6_t("2001:db8::/32") != prefix6_t("2001:db9::/32"))  == true,  "ipv6 not equal");

  // --- Parse errors ---

  std::cout << "\n--- Parse errors ---\n";

  prefix6_t p6_err1;
  p6_err1.from_str ("2001:db8::/129", &ok);
  CHECK (ok == false, "ipv6 error: prefix > 128");

  prefix6_t p6_err2;
  p6_err2.from_str ("not_ipv6/32", &ok);
  CHECK (ok == false, "ipv6 error: invalid IP");

  prefix6_t p6_err3;
  p6_err3.from_str ("::/", &ok);
  CHECK (ok == false, "ipv6 error: slash without number");

  // no slash — host prefix
  prefix6_t p6_ns;
  p6_ns.from_str ("::1", &ok);
  CHECK (ok == true,                        "ipv6 no slash success");
  CHECK (p6_ns.length == 128,               "ipv6 no slash /128");
  CHECK (p6_ns.ip == ip6_t("::1"),          "ipv6 no slash value");

  // --- Hash ---

  std::cout << "\n--- Hash ---\n";

  std::unordered_set<prefix6_t> set6;
  set6.insert (prefix6_t("2001:db8::/32"));
  set6.insert (prefix6_t("fe80::/10"));
  set6.insert (prefix6_t("2001:db8::/32")); // duplicate

  CHECK (set6.size () == 2,                                     "ipv6 hash set size");
  CHECK (set6.count (prefix6_t("2001:db8::/32")) == 1,          "ipv6 hash set find");
  CHECK (set6.count (prefix6_t("::/0")) == 0,                   "ipv6 hash set not find");


  std::cout << "\n========================================\n";
  std::cout << "  Generic prefix_t<> Tests\n";
  std::cout << "========================================\n\n";

  prefix_t<v4> gen4 = "10.0.0.0/8";
  prefix_t<v6> gen6 = "2001:db8::/32";

  std::cout << "gen4: " << gen4 << '\n';
  std::cout << "gen6: " << gen6 << '\n';

  CHECK (gen4.to_str () == "10.0.0.0/8",      "generic v4");
  CHECK (gen6.to_str () == "2001:db8::/32",    "generic v6");
  CHECK (gen4.contains (ip4_t("10.1.2.3")),    "generic v4 contains");
  CHECK (gen6.contains (ip6_t("2001:db8::1")), "generic v6 contains");


  // --- Bit-level with IPv6 ---

  std::cout << "\n--- Bit-level with IPv6 ---\n";

  prefix6_t p6_bits;
  // Build 0x20 = 0b00100000 as first byte (= "2" in "2001:...")
  p6_bits.push_back_bit (0);
  p6_bits.push_back_bit (0);
  p6_bits.push_back_bit (1);
  p6_bits.push_back_bit (0);
  p6_bits.push_back_bit (0);
  p6_bits.push_back_bit (0);
  p6_bits.push_back_bit (0);
  p6_bits.push_back_bit (0);

  std::cout << "p6_bits: " << p6_bits << '\n';
  CHECK (p6_bits.length == 8,         "ipv6 push_back_bit length");
  CHECK (p6_bits.ip[0] == 0x20,       "ipv6 push_back_bit byte[0] = 0x20");
  CHECK (p6_bits.get_bit (0) == false, "ipv6 get_bit 0");
  CHECK (p6_bits.get_bit (2) == true,  "ipv6 get_bit 2");


  std::cout << "\n========================================\n";
  if (failures == 0)
    std::cout << "  All tests PASSED!\n";
  else
    std::cout << "  " << failures << " test(s) FAILED!\n";
  std::cout << "========================================\n";

  return failures;

}
