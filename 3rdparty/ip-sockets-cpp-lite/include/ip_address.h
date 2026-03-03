
/*
 * ip-sockets-cpp-lite — header-only C++ networking utilities
 * https://github.com/biaks/ip-sockets-cpp-lite
 * 
 * Copyright (c) 2021 Yan Kryukov ianiskr@gmail.com
 * Licensed under the MIT License
 */

#pragma once

// ORDERS BEGIN

#include <type_traits>
#include <cstdint>

#if defined(_MSC_VER)

  #include <stdlib.h>
  #define COMMON_LITTLE_ENDIAN 1
  
  #define bswap_16(x) _byteswap_ushort(x)
  #define bswap_32(x) _byteswap_ulong(x)
  #define bswap_64(x) _byteswap_uint64(x)

#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

  #define COMMON_LITTLE_ENDIAN 1
  
  #define bswap_16(x) __builtin_bswap16(x)
  #define bswap_32(x) __builtin_bswap32(x)
  #define bswap_64(x) __builtin_bswap64(x)

#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

  #define COMMON_LITTLE_ENDIAN 0
  
  #define bswap_16(x) __builtin_bswap16(x)
  #define bswap_32(x) __builtin_bswap32(x)
  #define bswap_64(x) __builtin_bswap64(x)

#else

  #error "Unsupported platform endian detection"

#endif

namespace orders {

  // network -> host
  template <typename T> inline std::enable_if_t<sizeof(T) == 1, T> ntohT (const T& value) { return value; }
  template <typename T> inline std::enable_if_t<sizeof(T) == 2, T> ntohT (const T& value) { return (COMMON_LITTLE_ENDIAN) ? bswap_16(value) : value; }
  template <typename T> inline std::enable_if_t<sizeof(T) == 4, T> ntohT (const T& value) { return (COMMON_LITTLE_ENDIAN) ? bswap_32(value) : value; }
  template <typename T> inline std::enable_if_t<sizeof(T) == 8, T> ntohT (const T& value) { return (COMMON_LITTLE_ENDIAN) ? bswap_64(value) : value; }

  // host -> network
  template <typename T> inline std::enable_if_t<sizeof(T) == 1, T> htonT (const T& value) { return value; }
  template <typename T> inline std::enable_if_t<sizeof(T) == 2, T> htonT (const T& value) { return (COMMON_LITTLE_ENDIAN) ? bswap_16(value) : value; }
  template <typename T> inline std::enable_if_t<sizeof(T) == 4, T> htonT (const T& value) { return (COMMON_LITTLE_ENDIAN) ? bswap_32(value) : value; }
  template <typename T> inline std::enable_if_t<sizeof(T) == 8, T> htonT (const T& value) { return (COMMON_LITTLE_ENDIAN) ? bswap_64(value) : value; }

} // namespace common

// ORDERS END

#include <array>
#include <iostream>
#include <string>
#include <cassert>
#include <algorithm>   // std::copy_n
#include <functional>  // std::hash

// base classes for working with ipv4 and ipv6 addresses classes are based on standard std::array<uint, type>
// and can be "overlayed" onto corresponding memory areas, for example:
// uint8_t fragment[10] = {0xff,  0xc0, 0xa8, 0x02, 0x01,  0xff,0xff,0xff,0xff,0xff};
// ip4_t& ipv4 = *((ip4_t*)&fragment[1]);
// after that, all tools for working with this ip address become available, without copying the ip address itself
//
// also, classes can be declared in two different ways:
// use ip4_t and ip6_t classes directly where it is known in advance which ip address type will be used
// or use the generic form ip_t<type> where working with an abstract ip address is intended, for example:
// template <ip_type_e type>
// struct ip_address_holder_t {
//   ip_t<type> ip_address;
// };
//
// also, this universal form allows reverse type deduction and determining, for example, the ip address type:
// template <ip_type_e type>
// struct ip_detector;
// template <>
// ip_detector<ip_t<v4>> {
//   static inline const char* type() { return "ipv4"; }
// }
// ip_detector<ip_t<v6>> {
//   static inline const char* type() { return "ipv6"; }
// }
// template <class Ip>  
// void print_type () { std::cout << ip_detector<Ip>::type(); }

namespace ipsockets {

  struct ip4_t : public std::array<uint8_t, 4> {
    /// @brief Parses a text string with an IP address according to the rules.
    /// The string can contain from one to four numbers separated by dots.
    /// Each number can be encoded as HEX (hexadecimal number) or as DEC (decimal number).
    /// If there is one number, it is interpreted as uint32 (i.e., one number contains the entire ip address).
    /// If there are 2 to 4 numbers, they are interpreted as values of individual address octets, empty octets are filled with zeros.
    /// The IP address value is stored in the std::array<uint8_t, 4> array from high byte to low byte (big endian).
    /// If parsing fails, the result will be 0.0.0.0. To get confirmation of successful parsing, pass a pointer to a bool variable
    /// as the success parameter, which will be set to true in case of successful parsing.
    /// @param value   - pointer to the string with the ip address, it can be null-terminated or not, in the second case, the length parameter should be set correctly.
    /// @param length  - length of the string with the ip address, if the string is null-terminated, then this parameter can be left as default.
    /// @param success - optional parameter for confirming successful parsing.
    /// @return reference to the current object with the parsed ip address, in case of parsing failure, the result will be 0.0.0.0
    /// @details Possibly examples:
    /// "192.168.2.1"
    /// "192.0xa8.2.0x1"
    /// "0xc0a80201"
    /// "3232236033"
    /// "127.1"
    ip4_t& from_str (const char* value, size_t length = 20, bool* success = nullptr) {
      enum { start, cont, hex, dec, error } state = start;

      uint8_t  result[3] = {};
      size_t   octet     = 0;
      uint32_t accum     = 0;

      while (length-- && *value != '\0' && state != error) {

        if (state == start) {
          if (*value < '0' || '9' < *value)
            state = error;
          else if (*value == '0' && length >= 2 && (*(value + 1) == 'x' || *(value + 1) == 'X')) {
            value  += 2;
            length -= 2;
            state   = hex;
          }
          else
            state   = dec;
        }

        if (state == dec) {
          if ('0' <= *value && *value <= '9')
            accum = accum * 10 + (*value - '0');
          else if (*value == '.')
            state = cont;
          else
            state = error;
        }
        else if (state == hex) {
          if      ('0' <= *value && *value <= '9') accum = (accum << 4) + (     *value - '0');
          else if ('a' <= *value && *value <= 'f') accum = (accum << 4) + (10 + *value - 'a');
          else if ('A' <= *value && *value <= 'F') accum = (accum << 4) + (10 + *value - 'A');
          else if (*value == '.')
            state = cont;
          else
            state = error;
        }

        if (state == cont) {
          if (accum > 0xff || octet >= 3)
            state = error;
          else {
            result[octet++] = (uint8_t)accum;
            accum           = 0;
            state           = start;
          }
        }

        value++;
      }

      if (state == error)
        *this = ip4_t {};
      else if (octet != 0) {
        if (accum > 0xff)
          *this = ip4_t {};
        else {

          // 1.2.3.4  1.2.x.3  1.x.x.2  1.x.x.x
          // 0 1 2 3  0 1 x 2  0 x x 1  0 x x x

          for (size_t i = 0; i < octet; i++)
            (*this)[i] = result[i];

          for (size_t i = octet; i < 3; i++)
            (*this)[i] = 0;

          (*this)[3] = (uint8_t)accum;
        }
      }
      else
        *((uint32_t*)this) = orders::htonT (accum);

      if (success)
        *success = (state != error);

      return *this;
    }

    /// @brief Parses a text string with an IP address according to the rules.
    /// The string can contain from one to four numbers separated by dots.
    /// Each number can be encoded as HEX (hexadecimal number) or as DEC (decimal number).
    /// If there is one number, it is interpreted as uint32 (i.e., one number contains the entire ip address).
    /// If there are 2 to 4 numbers, they are interpreted as values of individual address octets, empty octets are filled with zeros.
    /// The IP address value is stored in the std::array<uint8_t, 4> array from high byte to low byte (big endian).
    /// If parsing fails, the result will be 0.0.0.0. To get confirmation of successful parsing, pass a pointer to a bool variable
    /// as the success parameter, which will be set to true in case of successful parsing.
    /// @param value   - std::string with the ip address
    /// @param success - optional parameter for confirming successful parsing.
    /// @return reference to the current object with the parsed ip address, in case of parsing failure, the result will be 0.0.0.0
    /// @details Possibly examples:
    /// "192.168.2.1"
    /// "192.0xa8.2.0x1"
    /// "0xc0a80201"
    /// "3232236033"
    /// "127.1"
    ip4_t& from_str (const std::string& value, bool* success = nullptr) {
      return from_str (value.data (), value.size (), success);
    }

    ip4_t () = default;

    ip4_t (std::array<uint8_t, 4>&& arr) : std::array<uint8_t, 4> (arr) {}

    template <size_t Size>
    ip4_t (char (&&value)[Size]) {
      from_str (value, Size);
    }

    ip4_t (const char* value) {
      from_str (value);
    }

    ip4_t (const char* value, size_t length) {
      from_str (value, length);
    }

    ip4_t (const std::string& value) {
      from_str (value.data (), value.size ());
    }

    ip4_t (const uint32_t& value) {
      *((uint32_t*)this) = orders::htonT (value);
    }

    ip4_t (uint32_t&& value) {
      *((uint32_t*)this) = orders::htonT (value);
    }

    ip4_t (uint8_t&& v1, uint8_t&& v2, uint8_t&& v3, uint8_t&& v4) {
      (*this)[0] = v1; (*this)[1] = v2; (*this)[2] = v3; (*this)[3] = v4;
    }

    ip4_t (uint8_t prefix) {
      const uint8_t masks[9]  = { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };
      const uint8_t byte_size = 8;
      for (size_t i = 0; i < 4; ++i) {
        if (prefix >= byte_size) {
          this->operator[] (i) = masks[byte_size];
          prefix -= byte_size;
        }
        else {
          this->operator[] (i) = masks[prefix];
          prefix = 0;
        }
      }
    }

    ip4_t& set_mask (uint8_t prefix) {
      ip4_t mask (prefix);
      this->operator&= (mask);
      return *this;
    }

    ip4_t operator& (const ip4_t& other) {
      return *((uint32_t*)this) & *((uint32_t*)&other);
    }

    void operator&= (const ip4_t& other) {
      *((uint32_t*)this) &= *((uint32_t*)&other);
    }

    void rotate () {
      std::swap ((*this)[0], (*this)[3]);
      std::swap ((*this)[1], (*this)[2]);
    }

    /// @brief Converts ipv4 address to text representation.
    /// @return text representation of ipv4 address
    std::string to_str () const {
      return std::to_string ((*this)[0]) + '.' + std::to_string ((*this)[1]) + '.' + std::to_string ((*this)[2]) + '.' + std::to_string ((*this)[3]);
    }

    operator std::string () const {
      return to_str ();
    }

    operator uint32_t () const {
      return orders::ntohT (*((uint32_t*)this));
    }
  };

  static inline std::ostream& operator<< (std::ostream& os, const ip4_t& ipv4) {
    os << ipv4.to_str ();
    return os;
  }

} // namespace ipsockets

template<>
struct std::hash<ipsockets::ip4_t> {
  inline std::size_t operator() (const ipsockets::ip4_t& ip) const {
    return (uint32_t)ip;
  }
};

namespace ipsockets {

  struct ip6_t : public std::array<uint8_t, 16> {

    /// @brief Parses a text string with an IP address according to the rules.
    /// The string can contain from zero to eight HEX (hexadecimal) numbers separated by ':'.
    /// Additionally, compression of one or more zero numbers using '::' is supported
    /// Also, notation with an ipv4 address instead of the last two numbers is supported. The ipv4 address can only be written with decimal digits.
    /// If parsing fails, the result will be '::'. To get confirmation of successful parsing, pass a pointer to a bool variable
    /// as the success parameter, which will be set to true in case of successful parsing.
    /// @param value   - pointer to the string with the ip address, it can be null-terminated or not, in the second case, the length parameter should be set correctly.
    /// @param length  - length of the string with the ip address, if the string is null-terminated, then this parameter can be left as default.
    /// @param success - optional parameter for confirming successful parsing.
    /// @return reference to the current object with the parsed ip address, in case of parsing failure, the result will be '::'
    /// @details Possibly examples:
    /// "5555:6666:7777:8888:9999:aaaa:bbbb:cccc"
    /// "1:2:3:4:5:6:7:8"
    /// "1::5:6:7:8"
    /// "::5:6:7:8"
    /// "1:2:3:4::"
    /// "::"
    /// "1:2:3:4:5:6:192.168.2.1"
    /// "::192.168.2.1"
    /// "127.0.0.1" -> "::ffff:127.0.0.1"
    /// "5555:6666:7777:8888:9999:aaaa:255.255.255.255"
    ip6_t& from_str (const char* value, size_t length = 45, bool* success = nullptr) {
      enum { start, proc, error } state = start;

      uint16_t result_hex[8] = {};
      uint8_t  result_dec[3] = {};
      size_t   index_hex     = 0;
      size_t   index_dec     = 0;
      size_t   separator     = SIZE_MAX;
      uint32_t accum_dec     = 0;
      uint32_t accum_hex     = 0;
      uint8_t  sym           = 0;

      while (length-- && *value != '\0' && state != error) {

        // ::xx..  ..xx::xx..  ..xx::  ::
        if (length >= 1 && *value == ':' && *(value + 1) == ':') {
          if (separator == SIZE_MAX && index_dec == 0 && accum_hex <= 0xffff && index_hex < 7) {
            if (state == proc) {
              result_hex[index_hex++] = (uint16_t)accum_hex;
              accum_hex               = 0;
              accum_dec               = 0;
            }
            separator = index_hex;
            value  += 1;
            length -= 1;
          }
          else
            state = error;
        }
        // ..xx:xx..
        else if (*value == ':') {
          if (state == proc && index_dec == 0 && accum_hex <= 0xffff && index_hex < 7) {
            result_hex[index_hex++] = (uint16_t)accum_hex;
            accum_hex               = 0;
            accum_dec               = 0;
          }
          else
            state = error;
        }
        // ..xx:n.n.n.n  ..xx::n.n.n.n  ::n.n.n.n
        else if (*value == '.') {
          if (state == proc && accum_dec <= 0xff && index_dec <= 6 && index_dec < 3) {
            result_dec[index_dec++] = (uint8_t)accum_dec;
            accum_dec               = 0;
            accum_hex               = 0;
          }
          else
            state = error;
        }
        // xxxx
        else {
          if      ('0' <= *value && *value <= '9') sym = *value - '0';
          else if ('a' <= *value && *value <= 'f') sym = 10 + *value - 'a';
          else if ('A' <= *value && *value <= 'F') sym = 10 + *value - 'A';
          else
            state = error;

          accum_hex = (accum_hex << 4) + sym;
          accum_dec =  accum_dec * 10  + sym;
          if (state != error)
            state = proc;
        }

        value++;
      }

      if ((index_dec != 0 && index_dec != 3) || (index_dec == 0 && accum_hex > 0xffff) || (index_dec == 3 && accum_dec > 0xff))
        state = error;

      if (state == error) {
        *this = {};
        if (success)
          *success = false;
        return *this;
      }

      if (index_dec == 3) { // found an ipv4 address in the end, then the last two numbers of ipv6 will be formed from this ipv4 address
        if (index_hex == 0 && separator == SIZE_MAX) // if there is nothing at front of ipv4 address, then the ipv4 address shuld be written as ipv4 over ipv6 ::ffff:x.x.x.x
          result_hex[index_hex++] = 0xffff;
        result_hex[index_hex++] = (result_dec[0] << 8) | result_dec[1];
        result_hex[index_hex++] = (result_dec[2] << 8) | (uint8_t)accum_dec;
      }
      else
        result_hex[index_hex++] = (uint16_t)accum_hex;



      size_t current = 0;

      if (separator == SIZE_MAX)
        separator = 0;

      // copy all numbers before separator (if separator is 0, then this loop will be skipped)
      for (; current < separator; current++)
        ((uint16_t*)this)[current] = orders::htonT (result_hex[current]);

      // fill zeros in place of separator
      for (; current < separator + (8 - index_hex); current++)
        ((uint16_t*)this)[current] = 0x0000;

      // copy all numbers after separator
      for (; current < 8; current++)
        ((uint16_t*)this)[current] = orders::htonT (result_hex[current - (8 - index_hex)]);

      if (success)
        *success = true;
      return *this;
    }

    /// @brief Parses a text string with an IP address according to the rules.
    /// The string can contain from zero to eight HEX (hexadecimal) numbers separated by ':'.
    /// Additionally, compression of one or more zero numbers using '::' is supported
    /// Also, notation with an ipv4 address instead of the last two numbers is supported. The ipv4 address can only be written with decimal digits.
    /// If parsing fails, the result will be '::'. To get confirmation of successful parsing, pass a pointer to a bool variable
    /// as the success parameter, which will be set to true in case of successful parsing.
    /// @param value   - string with the ip address.
    /// @param success - optional parameter for confirming successful parsing.
    /// @return reference to the current object with the parsed ip address, in case of parsing failure, the result will be '::'
    /// @details Possibly examples:
    /// "5555:6666:7777:8888:9999:aaaa:bbbb:cccc"
    /// "1:2:3:4:5:6:7:8"
    /// "1::5:6:7:8"
    /// "::5:6:7:8"
    /// "1:2:3:4::"
    /// "::"
    /// "1:2:3:4:5:6:192.168.2.1"
    /// "::192.168.2.1"
    /// "127.0.0.1" -> "::ffff:127.0.0.1"
    /// "5555:6666:7777:8888:9999:aaaa:255.255.255.255"
    ip6_t& from_str (const std::string& value, bool* success = nullptr) {
      return from_str (value.data (), value.size (), success);
    }

    ip6_t () = default;

    ip6_t (std::array<uint8_t, 16>&& arr) : std::array<uint8_t, 16> (arr) {}


    // For translating ipv4 addresses to ipv6, we choose the method supported by ClickHouse ::ffff:x.x.x.x/96 rfc4291 https://www.iana.org/go/rfc4291
    ip6_t (std::array<uint8_t, 4>&& ipv4) : std::array<uint8_t, 16> () {
      (*this)[10] = 0xff; (*this)[11] = 0xff; (*this)[12] = ipv4[0]; (*this)[13] = ipv4[1]; (*this)[14] = ipv4[2]; (*this)[15] = ipv4[3];
    }

    // For translating ipv4 addresses to ipv6, we choose the method supported by ClickHouse ::ffff:x.x.x.x/96 rfc4291 https://www.iana.org/go/rfc4291
    ip6_t (const ip4_t& ipv4) : std::array<uint8_t, 16> () {
      (*this)[10] = 0xff; (*this)[11] = 0xff; (*this)[12] = ipv4[0]; (*this)[13] = ipv4[1]; (*this)[14] = ipv4[2]; (*this)[15] = ipv4[3];
    }

    template <size_t Size>
    ip6_t (char (&&value)[Size]) {
      from_str (value, Size);
    }

    ip6_t (const char* value) {
      from_str (value);
    }

    ip6_t (const char* value, size_t length) {
      from_str (value, length);
    }

    ip6_t (const std::string& value) {
      from_str (value.data (), value.size ());
    }

    ip6_t (uint8_t&& v1, uint8_t&& v2,  uint8_t&& v3,  uint8_t&& v4,  uint8_t&& v5,  uint8_t&& v6,  uint8_t&& v7,  uint8_t&& v8,
           uint8_t&& v9, uint8_t&& v10, uint8_t&& v11, uint8_t&& v12, uint8_t&& v13, uint8_t&& v14, uint8_t&& v15, uint8_t&& v16) {
      ((uint8_t*)this)[0]  = orders::htonT (v1);  ((uint8_t*)this)[1]  = orders::htonT (v2); 
      ((uint8_t*)this)[2]  = orders::htonT (v3);  ((uint8_t*)this)[3]  = orders::htonT (v4);
      ((uint8_t*)this)[4]  = orders::htonT (v5);  ((uint8_t*)this)[5]  = orders::htonT (v6);
      ((uint8_t*)this)[6]  = orders::htonT (v7);  ((uint8_t*)this)[7]  = orders::htonT (v8);
      ((uint8_t*)this)[8]  = orders::htonT (v9);  ((uint8_t*)this)[9]  = orders::htonT (v10);
      ((uint8_t*)this)[10] = orders::htonT (v11); ((uint8_t*)this)[11] = orders::htonT (v12);
      ((uint8_t*)this)[12] = orders::htonT (v13); ((uint8_t*)this)[13] = orders::htonT (v14);
      ((uint8_t*)this)[14] = orders::htonT (v15); ((uint8_t*)this)[15] = orders::htonT (v16);
    }

    ip6_t (uint16_t&& v1, uint16_t&& v2, uint16_t&& v3, uint16_t&& v4, uint16_t&& v5, uint16_t&& v6, uint16_t&& v7, uint16_t&& v8) {
      ((uint16_t*)this)[0] = orders::htonT (v1); ((uint16_t*)this)[1] = orders::htonT (v2);
      ((uint16_t*)this)[2] = orders::htonT (v3); ((uint16_t*)this)[3] = orders::htonT (v4);
      ((uint16_t*)this)[4] = orders::htonT (v5); ((uint16_t*)this)[5] = orders::htonT (v6);
      ((uint16_t*)this)[6] = orders::htonT (v7); ((uint16_t*)this)[7] = orders::htonT (v8);
    }

    const ip4_t& get_ip4 () const {
      return *((ip4_t*)this + 3); // 4*3 = 12
    }

    ip4_t& get_ip4 () {
      return *((ip4_t*)this + 3); // 4*3 = 12
    }

    bool is_ip4() const {

      uint32_t first = ((const uint32_t*)this)[0];
      uint32_t second = ((uint32_t*)this)[1];
      uint32_t thrird = ((uint32_t*)this)[2];

      return first == 0 &&
             second == 0 &&
             thrird == 0xffff0000;
    }

    ip6_t (uint8_t prefix) {
      const uint8_t masks[9]  = { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };
      const uint8_t byte_size = 8;
      for (size_t i = 0; i < 16; ++i) {
        if (prefix >= byte_size) {
          this->operator[] (i) = masks[byte_size];
          prefix -= byte_size;
        } else {
          this->operator[] (i) = masks[prefix];
          prefix               = 0;
        }
      }
    }

    ip6_t& set_mask (uint8_t prefix) {
      ip6_t mask (prefix);
      this->operator&= (mask);
      return *this;
    }

    /// @brief Converts ipv6 address to text representation.
    /// Supports optional address compression and optional output of the last two groups as an IPv4 address,
    /// always outputting the special case of ip4-over-ip6 as '::ffff:x.x.x.x' string.
    /// @param reduction - enable address compression (replacing empty groups with '::')
    /// @param embedded_ipv4 - enable force representation of last two groups as ipv4 address
    /// @return text representation of ipv6 address
    std::string to_str (bool reduction = true, bool embedded_ipv4 = false) const {

      std::string                result;
      const char                 symbols[] = "0123456789abcdef";
      bool                       dot       = false;
      const uint16_t*            part      = (uint16_t*)this;
      enum { start, found, end } zero      = (reduction) ? start : end;

      result.reserve (45); // maximum length "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:nnn.nnn.nnn.nnn" = 45

      for (size_t i = 0; i < 8; i++, part++) {
        if (*part == 0 && zero == start) {
          zero = found;
          result.push_back (':');
          if (dot == false) result.push_back (':');
        }
        else
          if (*part != 0 && zero == found)
            zero = end;
        if (zero != found) {
          bool    non_zero = false;
          uint8_t half_of_byte;
          half_of_byte = *((uint8_t*)part    ) >> 4; if (half_of_byte) non_zero = true; if (non_zero) result.push_back (symbols[half_of_byte]);
          half_of_byte = *((uint8_t*)part    ) & 15; if (half_of_byte) non_zero = true; if (non_zero) result.push_back (symbols[half_of_byte]);
          half_of_byte = *((uint8_t*)part + 1) >> 4; if (half_of_byte) non_zero = true; if (non_zero) result.push_back (symbols[half_of_byte]);
          half_of_byte = *((uint8_t*)part + 1) & 15;                                                  result.push_back (symbols[half_of_byte]);
          dot = true;
          if (i != 7)
            result.push_back (':');
        }
        if ((i == 4 && zero == found && *(part + 1) == 0xffff) || (embedded_ipv4 && i == 5)) {
          if (i == 4) {
            for (size_t i = 0; i < 4; i++)
              result.push_back('f');
            result.push_back(':');
            part += 2;
          }
          else
            part += 1;
          for (size_t i = 0; i < 4; i++) {
            uint8_t byte     = *((uint8_t*)part + i); // byte = XYZ
            uint8_t div      = byte % 100;            // div = XYZ % 100 = 0YZ
            bool    non_zero = false;
            byte /= 100; // byte = XYZ / 100 = X
            if (byte != 0) {
              result.push_back (symbols[byte]);
              non_zero = true;
            }
            byte = div / 10; // byte = 0YZ / 10 = Y
            div  = div % 10; // div = 0YZ % 10 = Z
            if (byte != 0 || non_zero == true)
              result.push_back (symbols[byte]);
            result.push_back (symbols[div]);
            if (i != 3)
              result.push_back ('.');
          }
          break;
        }
      }
      return result;
    }

    operator std::string () const {
      return to_str ();
    }

    operator bool () const {
      const uint64_t* part1 = (uint64_t*)this;
      const uint64_t* part2 = part1 + 1;
      return (*part1 || *part2);
    }

    ip6_t operator& (const ip6_t& other_ip) {
      ip6_t result;
      *((uint64_t*)&result)     = *((uint64_t*)this)     & *((uint64_t*)&other_ip);
      *((uint64_t*)&result + 1) = *((uint64_t*)this + 1) & *((uint64_t*)&other_ip + 1);
      return result;
    }

    void operator&= (const ip6_t& other_ip) {
      *((uint64_t*)this)     &= *((uint64_t*)&other_ip);
      *((uint64_t*)this + 1) &= *((uint64_t*)&other_ip + 1);
    }

  };

  static inline std::ostream& operator<< (std::ostream& os, const ip6_t& ipv6) {
    os << ipv6.to_str ();
    return os;
  }

} // namespace ipsockets

template <>
struct std::hash<ipsockets::ip6_t> {
  inline std::size_t operator() (const ipsockets::ip6_t& ip) const noexcept{
    return *((uint64_t*)&ip) ^ *((uint64_t*)&ip + 1);
  }
};

namespace ipsockets {

  enum ip_type_e { v4 = 4, v6 = 16 };

  template <ip_type_e type>
  struct ip_t_;

  template <>
  struct ip_t_<v4> {
    using type = ip4_t;
  };

  template <>
  struct ip_t_<v6> {
    using type = ip6_t;
  };

  template <ip_type_e type>
  using ip_t = typename ip_t_<type>::type;

  // addr_t

  struct addr4_t {

    ip4_t    ip   = {};
    uint16_t port = 0; // le

    // nnn.nnn.nnn.nnn:ppppp
    // any_format_supported_by_ip4_t_class:ppppp
    addr4_t& from_str (const char* value, size_t length = 21, bool* success = nullptr) {

      size_t      ip_length = 0;
      const char* ptr     = value;
      size_t      accum   = 0;

      while (*ptr && length--) {
        if (ip_length == 0) {
          if (*ptr == ':')
            ip_length = ptr - value;
          else if ((*ptr < '0' || '9' < *ptr) && (*ptr < 'a' || 'f' < *ptr) && (*ptr < 'A' || 'F' < *ptr) && *ptr != 'x' && *ptr != '.')
            break; // this is error, break
        }
        else {
          if ('0' <= *ptr && *ptr <= '9')
            accum = accum * 10 + (*ptr - '0');
          else
            break;
        }
        ptr++;
      }

      ip.from_str (value, ip_length, success);

      if (ip_length == 0 || accum == 0 || accum > 0xffff) {
        if (success)
          *success = false;
        *this = {};
      }
      else    
        port = (uint16_t)accum;

      return *this;

    }

    addr4_t () = default;

    addr4_t (const ip4_t& ip_, uint16_t port_) : ip (ip_), port (port_) {}

    template <size_t Size>
    addr4_t (char (&&value)[Size]) {
      from_str (value, Size);
    }

    addr4_t (const char* value) {
      from_str (value);
    }

    addr4_t (const char* value, size_t length) {
      from_str (value, length);
    }

    addr4_t (const std::string& value) {
      from_str (value.data (), value.size ());
    }

    template <size_t Size>
    addr4_t (const std::array<uint8_t, Size>& value) {
      from_str (value.data(), Size);
    }

    std::string to_str () const {
      return ip.to_str() + ':' + std::to_string (port);
    }

    operator std::string () const {
      return to_str ();
    }

    operator bool () const {
      return (ip && port != 0);
    }

    bool operator== (const addr4_t& other_addr) const {
      return this->ip == other_addr.ip && this->port == other_addr.port;
    }

  };

  static inline std::ostream& operator<< (std::ostream& os, const addr4_t& addr4) {
    os << addr4.to_str ();
    return os;
  }

} // namespace ipsockets

template <>
struct std::hash<ipsockets::addr4_t> {
  std::size_t operator() (const ipsockets::addr4_t& addr4) const noexcept {
    return std::hash<ipsockets::ip4_t> {}(addr4.ip) ^ std::hash<uint16_t> {}(addr4.port);
  }
};

namespace ipsockets {

  struct addr6_t {

    ip6_t    ip;
    uint16_t port; // le

    // [xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:nnn.nnn.nnn.nnn]:ppppp
    // [any_format_supported_by_ip6_t_class]:ppppp
    addr6_t& from_str (const char* value, size_t length = 53, bool* success = nullptr) {

      enum { open, ipp, close, tochki, portt, error, end } state = open;
      const char* ip_ptr    = nullptr;
      size_t      ip_length = 0;
      const char* ptr       = value;
      size_t      accum     = 0;

      while (*ptr && length-- && state != error) {
        switch (state) {
          case open:
            if (*ptr == '[') {
              ip_ptr = ptr + 1;
              state  = ipp;
            }
            else
              state = error;
          break;
          case ipp:
            if (*ptr == ']') {
              ip_length = ptr - ip_ptr;
              if (ip_length >= 2)
                state = close;
              else
                state = error;
            }
            else if ((*ptr < '0' || '9' < *ptr) && (*ptr < 'a' || 'f' < *ptr) && (*ptr < 'A' || 'F' < *ptr) && *ptr != ':' && *ptr != '.')
              state = error;
          break;
          case close:
            if (*ptr == ':')
              state  = portt;
            else
              state = error;
          break;
          case portt:
            if ('0' <= *ptr && *ptr <= '9')
              accum = accum * 10 + (*ptr - '0');
            else if (accum != 0)
              state = end;
            else
              state = error;
          break;
          default:
          break;
        }
        ptr++;
      }

      if (state != error)
        ip.from_str (ip_ptr, ip_length, success);

      if (state == error || accum > 0xffff) {
        if (success)
          *success = false;
        *this = {};
      }
      else
        port = (uint16_t)accum;

      return *this;

    }

    addr6_t () = default;

    addr6_t (const ip6_t& ip_, uint16_t port_) : ip (ip_), port (port_) {}

    template <size_t Size>
    addr6_t (char (&&value)[Size]) {
      from_str (value, Size);
    }

    addr6_t (const char* value) {
      from_str (value);
    }

    addr6_t (const char* value, size_t length) {
      from_str (value, length);
    }

    addr6_t (const std::string& value) {
      from_str (value.data (), value.size ());
    }

    template <size_t Size>
    addr6_t (const std::array<uint8_t, Size>& value) {
      from_str (value.data (), Size);
    }

    std::string to_str (bool reduction = true, bool embedded_ipv4 = false) const {
      return std::string ("[") + ip.to_str (reduction, embedded_ipv4) + std::string ("]:") + std::to_string (port);
    }

    operator std::string () const {
      return to_str ();
    }

    operator bool () const {
      return (ip && port != 0);
    }

    bool operator== (const addr6_t& other_addr) const {
      return this->ip == other_addr.ip && this->port == other_addr.port;
    }

  };

  static inline std::ostream& operator<< (std::ostream& os, const addr6_t& addr6) {
    os << addr6.to_str ();
    return os;
  }

} // namespace ipsockets

template <>
struct std::hash<ipsockets::addr6_t> {
  inline std::size_t operator() (const ipsockets::addr6_t& addr6) const noexcept {
    return std::hash<ipsockets::ip6_t> {}(addr6.ip) ^ std::hash<uint16_t> {}(addr6.port);
  }
};

namespace ipsockets {

  template <ip_type_e type>
  struct addr_t_;

  template <>
  struct addr_t_<v4> {
    using type = addr4_t;
  };

  template <>
  struct addr_t_<v6> {
    using type = addr6_t;
  };

  template <ip_type_e type>
  using addr_t = typename addr_t_<type>::type;


  // ============================================================
  // ip_prefix_raw_t — variable-length prefix overlay (C-style)
  // ============================================================

  /// @brief Raw variable-length IP prefix for zero-copy overlay on packed binary data.
  /// Layout: [1 byte length_in_bits] [ceil(length/8) bytes of prefix value]
  /// @note Uses flexible array member (compiler extension supported by GCC, Clang, MSVC).
  ///   Do NOT create instances on the stack — use reinterpret_cast on a sufficiently large buffer.
  #if defined(_MSC_VER)
  #pragma warning(push)
  #pragma warning(disable: 4200) // nonstandard extension: zero-sized array
  #endif
  #if defined(__GNUC__) || defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wpedantic"
  #endif
  struct ip_prefix_raw_t {
    uint8_t length;   ///< Prefix length in bits (0..128)
    uint8_t bytes[];  ///< Prefix value bytes (actual size = value_size())

    /// @brief Returns the number of bytes needed to store 'length' bits.
    uint8_t value_size () const {
      return (uint8_t)((length >> 3) + ((length & 0x7) ? 1 : 0));
    }

    /// @brief Returns the total size of this prefix in bytes (1 byte for length + value bytes).
    uint8_t size () const {
      return (uint8_t)(1 + value_size ());
    }

    /// @brief Returns the value of a specific bit in the prefix (MSB-first order within each byte).
    /// @param index - Bit index (0-based, must be < length).
    bool get_bit (uint8_t index) const {
      assert (index < length);
      return (bytes[index >> 3] >> (7 - (index & 0x7))) & 0x1;
    }
  };
  #if defined(__GNUC__) || defined(__clang__)
  #pragma GCC diagnostic pop
  #endif
  #if defined(_MSC_VER)
  #pragma warning(pop)
  #endif

  // ============================================================
  // prefix4_t / prefix6_t / prefix_t<> — IP prefix (ip + length)
  // ============================================================

  /// @brief IP network prefix: an IP address combined with a prefix length (CIDR notation).
  /// @details Represents a network prefix like "192.168.1.0/24" or "2001:db8::/32".
  ///   The IP field stores the prefix value (network address with host bits masked),
  ///   and the length field stores the number of significant bits.
  ///
  ///   Supports construction from:
  ///   - IP address only (assumes full host prefix: /32 for IPv4, /128 for IPv6)
  ///   - IP address + prefix length
  ///   - CIDR string ("192.168.1.0/24" or "2001:db8::/32")
  ///   - Raw binary prefix (ip_prefix_raw_t)
  ///
  ///   Also supports:
  ///   - Bit-level access (get_bit)
  ///   - Bit-level construction (push_back_bit, operator<<)
  ///   - Containment check (contains)
  ///   - Network address extraction (network)
  ///   - CIDR string conversion (to_str)
  ///   - Comparison and hashing
  ///   - Conversion to raw overlay (operator const ip_prefix_raw_t&)
  ///
  /// @code
  ///   prefix4_t p1 = "192.168.1.0/24";
  ///   prefix4_t p2 (24, ip4_t("192.168.1.100"));
  ///   std::cout << p1;                       // "192.168.1.0/24"
  ///   std::cout << p1.contains("192.168.1.5"); // true
  ///   std::cout << p1.network();             // "192.168.1.0"
  ///
  ///   prefix6_t p3 = "2001:db8::/32";
  ///   prefix_t<v4> p4 = "10.0.0.0/8";       // generic form
  /// @endcode
  template <ip_type_e Ip_type>
  struct ip_prefix_t {

    static const uint8_t max_length = (uint8_t)(Ip_type * 8); ///< Maximum prefix length in bits (32 for IPv4, 128 for IPv6)

    uint8_t      length = 0;  ///< Prefix length in bits (0..max_length)
    ip_t<Ip_type> ip    = {}; ///< Prefix value (network address, host bits should be zero)

    // ===== constructors =====

    ip_prefix_t () = default;

    /// @brief Constructs a host prefix from an IP address (length = max: /32 or /128).
    /// @param ip_ - IP address to use as a full host prefix.
    ip_prefix_t (const ip_t<Ip_type>& ip_)
      : length (max_length), ip (ip_) {}

    /// @brief Constructs a prefix from a length and IP address.
    /// @param t_length - Prefix length in bits.
    /// @param ip_     - IP address (host bits beyond t_length are masked off).
    /// @details The IP address is automatically masked to the given prefix length,
    ///   so ip_prefix_t(24, "192.168.1.100") produces "192.168.1.0/24".
    ip_prefix_t (const ip_t<Ip_type>& ip_, uint8_t length_)
      : length (length_) {
      assert (length_ <= max_length);
      ip = ip_;
      apply_mask ();
    }

    /// @brief Constructs a prefix from a raw binary prefix overlay.
    /// @param prefix_raw - Raw prefix (length + bytes).
    ip_prefix_t (const ip_prefix_raw_t& prefix_raw)
      : length (prefix_raw.length) {
      assert (length <= max_length);
      uint8_t bytes_to_copy = prefix_raw.value_size ();
      if (bytes_to_copy > Ip_type) bytes_to_copy = Ip_type;
      std::copy_n (prefix_raw.bytes, bytes_to_copy, ip.begin ());
    }

    /// @brief Constructs a prefix from a CIDR notation string.
    /// @param value - String in CIDR notation (e.g. "192.168.1.0/24" or "2001:db8::/32").
    /// @details Supported formats:
    ///   - IPv4: "192.168.1.0/24", "10.0.0.0/8"
    ///   - IPv6: "2001:db8::/32", "fe80::/10"
    ///   - Without prefix length: "192.168.1.1" is treated as /32, "::1" as /128
    ip_prefix_t (const char* value) {
      from_str (value);
    }

    /// @brief Constructs a prefix from a CIDR notation string.
    ip_prefix_t (const std::string& value) {
      from_str (value.data (), value.size ());
    }

    // ===== parsing =====

    /// @brief Parses a CIDR notation string into this prefix.
    /// @param value   - Pointer to the string.
    /// @param str_len - Length of the string (default: large enough for any address).
    /// @param success - Optional output flag for parsing success.
    /// @return Reference to this object.
    /// @details Format: "ip_address/prefix_length" or just "ip_address" (assumes max length).
    ///   On parse failure, the prefix is reset to 0/0 (empty).
    ip_prefix_t& from_str (const char* value, size_t str_len = 50, bool* success = nullptr) {
      // find '/' separator
      const char* ptr         = value;
      size_t      remaining   = str_len;
      size_t      ip_len      = 0;
      bool        found_slash = false;

      while (remaining-- && *ptr != '\0') {
        if (*ptr == '/') {
          ip_len      = (size_t)(ptr - value);
          found_slash = true;
          ptr++;
          break;
        }
        ptr++;
      }

      if (!found_slash) {
        // no slash — treat as host prefix
        bool ip_ok = false;
        ip.from_str (value, str_len, &ip_ok);
        if (ip_ok) {
          length = max_length;
          if (success) *success = true;
        }
        else {
          *this = {};
          if (success) *success = false;
        }
        return *this;
      }

      // parse IP part
      bool ip_ok = false;
      ip.from_str (value, ip_len, &ip_ok);

      if (!ip_ok) {
        *this = {};
        if (success) *success = false;
        return *this;
      }

      // parse prefix length
      uint32_t accum = 0;
      bool     has_digits = false;
      while (*ptr != '\0' && remaining-- > 0) {
        if ('0' <= *ptr && *ptr <= '9') {
          accum = accum * 10 + (uint32_t)(*ptr - '0');
          has_digits = true;
        }
        else
          break;
        ptr++;
      }

      if (!has_digits || accum > max_length) {
        *this = {};
        if (success) *success = false;
        return *this;
      }

      length = (uint8_t)accum;
      apply_mask ();

      if (success) *success = true;
      return *this;
    }

    /// @brief Parses a CIDR notation string into this prefix.
    ip_prefix_t& from_str (const std::string& value, bool* success = nullptr) {
      return from_str (value.data (), value.size (), success);
    }

    // ===== conversion =====

    /// @brief Converts the prefix to CIDR notation string (e.g. "192.168.1.0/24").
    std::string to_str () const {
      return ip.to_str () + '/' + std::to_string (length);
    }

    operator std::string () const {
      return to_str ();
    }

    /// @brief Converts to raw binary prefix overlay.
    /// @warning The returned reference points to the memory of this object.
    ///   It is valid as long as this object exists and its layout matches ip_prefix_raw_t.
    operator const ip_prefix_raw_t& () const {
      return *reinterpret_cast<const ip_prefix_raw_t*>(this);
    }

    // ===== bit-level operations =====

    /// @brief Sets the value of a specific bit in the prefix (MSB-first within each byte).
    /// @param index - Bit index (0-based, must be < length).
    /// @param value - Bit value to set (true = 1, false = 0).
    void set_bit (uint8_t index, bool value) {
      assert (index < length);
      uint8_t byte_index = index >> 3;
      uint8_t bit_index  = index & 0x7;
      uint8_t bit_mask   = (uint8_t)(0x80 >> bit_index);
      if (value) ip[byte_index] |=  bit_mask;
      else       ip[byte_index] &= ~bit_mask;
    }

    /// @brief Returns the value of a specific bit in the prefix (MSB-first within each byte).
    /// @param index - Bit index (0-based, must be < length).
    bool get_bit (uint8_t index) const {
      assert (index < length);
      uint8_t byte_index = index >> 3;
      uint8_t bit_index  = index & 0x7;
      return (ip[byte_index] >> (7 - bit_index)) & 0x1;
    }

    /// @brief Appends a single bit to the end of the prefix, incrementing length by 1.
    /// @param bit - Bit value (0 or 1).
    /// @return Reference to this object.
    ip_prefix_t& push_back_bit (uint8_t bit) {
      assert (length < max_length);
      uint8_t byte_index = length >> 3;
      uint8_t bit_index  = length & 0x7;
      uint8_t bit_mask   = (uint8_t)(0x80 >> bit_index);
      length++;
      if ( bit ) ip[byte_index] |=  bit_mask;
      else       ip[byte_index] &= ~bit_mask;
      return *this;
    }

    /// @brief Returns a new prefix with one bit appended.
    /// @param bit - Bit value (0 or 1).
    ip_prefix_t operator<< (uint8_t bit) const {
      ip_prefix_t result = *this;
      result.push_back_bit (bit);
      return result;
    }

    // ===== network operations =====

    /// @brief Returns the network address (IP with host bits zeroed out).
    ip_t<Ip_type> network () const {
      ip_t<Ip_type> result = ip;
      mask_ip (result, length);
      return result;
    }

    /// @brief Checks whether a given IP address belongs to this prefix.
    /// @param other_ip - IP address to check.
    /// @return true if other_ip falls within this prefix.
    bool contains (const ip_t<Ip_type>& other_ip) const {
      ip_t<Ip_type> masked      = other_ip;
      ip_t<Ip_type> self_masked = ip;
      mask_ip (self_masked, length);
      mask_ip (masked,      length);
      return masked == self_masked;
    }

    /// @brief Checks whether another prefix is entirely contained within this prefix.
    /// @param other - Prefix to check.
    /// @return true if other is a sub-prefix of this one (same network, equal or longer length).
    bool contains (const ip_prefix_t& other) const {
      if (other.length < length)
        return false;
      return contains (other.ip);
    }

    // ===== comparison =====

    bool operator== (const ip_prefix_t& other) const {
      return length == other.length && ip == other.ip;
    }

    bool operator!= (const ip_prefix_t& other) const {
      return !(*this == other);
    }

    bool operator< (const ip_prefix_t& other) const {
      if (length != other.length) return length < other.length;
      return ip < other.ip;
    }

    /// @brief Returns true if the prefix is non-empty (length > 0).
    explicit operator bool () const {
      return length > 0;
    }

  private:

    /// @brief Applies the prefix mask to the stored IP, zeroing out host bits.
    void apply_mask () {
      mask_ip (ip, length);
    }

    /// @brief Zeros out all bits in 'target' beyond position 'prefix_len'.
    static void mask_ip (ip_t<Ip_type>& target, uint8_t prefix_len) {
      uint8_t full_bytes     = prefix_len >> 3;
      uint8_t remaining_bits = prefix_len & 0x7;

      // partial byte: keep only the top 'remaining_bits' bits
      if (full_bytes < Ip_type) {
        if (remaining_bits > 0) {
          uint8_t mask = (uint8_t)(0xFF << (8 - remaining_bits));
          target[full_bytes] &= mask;
          full_bytes++;
        }
        // zero out all subsequent bytes
        for (uint8_t i = full_bytes; i < Ip_type; i++)
          target[i] = 0;
      }
    }

  };

  // ===== direct type aliases =====

  using prefix4_t = ip_prefix_t<v4>;  ///< IPv4 prefix (e.g. 192.168.1.0/24)
  using prefix6_t = ip_prefix_t<v6>;  ///< IPv6 prefix (e.g. 2001:db8::/32)

  // ===== generic type alias (template-friendly) =====

  template <ip_type_e type>
  using prefix_t = ip_prefix_t<type>;

  // ===== stream output =====

  template <ip_type_e Ip_type>
  static inline std::ostream& operator<< (std::ostream& os, const ip_prefix_t<Ip_type>& prefix) {
    os << prefix.to_str ();
    return os;
  }

} // namespace ipsockets

template <ipsockets::ip_type_e Ip_type>
struct std::hash<ipsockets::ip_prefix_t<Ip_type>> {
  std::size_t operator() (const ipsockets::ip_prefix_t<Ip_type>& prefix) const noexcept {
    std::size_t h = std::hash<uint8_t> {}(prefix.length);
    for (uint8_t i = 0; i < Ip_type; i++)
      h ^= std::hash<uint8_t> {}(prefix.ip[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
  }
};


// here is a lot of theory about IPv6 addresses
// and different ways of packing ipv4 addresses into ipv6 addresses

// about ipv6 addresses: https://en.wikipedia.org/wiki/IPv6_address
// https://www.ccexpert.us/routing-switching-2/ipv6-address-types.html

// UNICAST - represents the address of a specific interface (receiver). the packet is delivered to the specified interface
// ANYCAST - represents a group of interfaces (receivers). the packet is delivered to any one interface from the specified group
// MULTICAST - represents a group of interfaces (receivers). the packet is delivered to all interfaces from the specified group

// UNICAST
// ANYCAST
// network       subnet         interface
// 48 or more    16 or less     64
// XXXX:XXXX:XXXX: XXXX :XXXX:XXXX:XXXX:XXXX
// XXXX:XXXX:XXXX:XXXX  :XXXX:XXXX:XXXX:XXXX

// LOCAL
// prefix                        interface
// 10 = 0b1111111010            64
// fe80:0000:0000:0000  :XXXX:XXXX:XXXX:XXXX

// ::1/128 — The loopback address is a unicast localhost address
//           (corresponding to 127.0.0.1/8 in IPv4).
// fe80::/10 — Addresses in the link-local prefix are only valid and unique on a single link
//            (comparable to the auto-configuration addresses 169.254.0.0/16 of IPv4).
// fc00::/7 — Unique local addresses (ULAs) are intended for local communication[34]
//            (comparable to IPv4 private addresses 10.0.0.0/8, 172.16.0.0/12 and 192.168.0.0/16).

// TEREDO - is a tunneling mechanism for ipv6 internet access for ipv4 clients through ipv4 network https://www.rfc-editor.org/rfc/rfc4380

// Teredo IPv6 Address https://blog.ip2location.com/knowledge-base/what-is-teredo-tunneling/
// teredo prefix   teredo server       ~NAT port  ~client IP
// 2001:0000      :x.x.x.x       :8000 :port      :x.x.x.x

// Teredo ip6to4 https://blog.ip2location.com/knowledge-base/what-is-6to4-tunneling/
// teredo prefix ipv4 address  subnet
// 2001          :x.x.x.x     :XXXX   :0000:0000 :x.x.x.x

// Dedicated IPv6 addresses for internet. "2000::/16" up to 2001. "2001::/16" after 2001. "2002::/16" for routing from IPv6 to IPv4 on the internet.

// MULTICAST (old) RFC 2373
// prefix fig sc group
// 8       4   4  112
// ff      x   x  :XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX

// MULTICAST (new) RFC 3306, updated by RFC 7371
// prefix ff1 sc ff2 rsrvd plen prefix              group
// 8       4   4  4   4     8    64                   32
// ff      x   x :x   x     xx  :XXXX:XXXX:XXXX:XXXX :XXXX:XXXX

// transform ipv4 to ipv6:
// http://www.gestioip.net/cgi-bin/subnet_calculator.cgi
// https://www.iana.org/assignments/iana-ipv6-special-registry/iana-ipv6-special-registry.xml
// ::ffff:x.x.x.x/96                   - IPv4-mapped IPv6 address                            ::ffff:0:0/96   rfc4291 https://www.iana.org/go/rfc4291
// ::ffff:0:x.x.x.x/96                 - IPv4-translated IPv6 address
// 2002:x.x.x.x::                      - Additional 6to4 information                         2002::/16       rfc3056 https://www.iana.org/go/rfc3056
// 64:ff9b::x.x.x.x/96                 - IPv4-IPv6 Translatable Address - Global internet    64:ff9b::/96    rfc6052 https://www.iana.org/go/rfc6052
// 64:ff9b:1:ffff:ffff:ffff:x.x.x.x/48 - IPv4-IPv6 Translatable Address - Private internets  64:ff9b:1::/48  rfc8215 https://www.iana.org/go/rfc8215

// The prefix that we recommend has the particularity of being "checksum neutral".
// The sum of the hexadecimal numbers "0064" and "ff9b" is "ffff",
// i.e.,
// a value equal to zero in one's complement arithmetic. An IPv4-embedded IPv6 address constructed
// with this prefix will have the same one's complement checksum as the embedded IPv4 address.

// 127.0.0.1/8 -> ::1/128
// 169.254.0.0/16 -> fe80::/10
// 192.168.0.0/16 -> fc00::/7
// 10.0.0.0/8 -> fc00::/7

// rfc4291 https://www.rfc-editor.org/rfc/rfc4291
// 2.5.5.1.  IPv4-Compatible IPv6 Address
//   ::x.x.x.x - ipv4 address must be unique, this type is now deprecated
// 2.5.5.2.  IPv4-Mapped IPv6 Address (more details rfc4038 https://www.rfc-editor.org/rfc/rfc4038)
//   ::ffff:x.x.x.x -

// more about multicast addresses: https://en.wikipedia.org/wiki/Multicast_address#IPv6

// "IP" or "[IP]"
// "xxxx" can be shortened to a single value "x"
// ::
// ::xxxx
// xxxx::
// ::xxxx:xxxx:xxxx
// xxxx:xxxx:xxxx::
// xxxx::xxxx
// max length xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:nnn.nnn.nnn.nnn
// ipv4 a.b.c.d -> ipv6 ::ffff:a.b.c.d
