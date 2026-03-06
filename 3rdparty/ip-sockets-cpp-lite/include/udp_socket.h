/*	
 * ip-sockets-cpp-lite — header-only C++ networking utilities
 * https://github.com/biaks/ip-sockets-cpp-lite
 * 
 * Copyright (c) 2021 Yan Kryukov ianiskr@gmail.com
 * Licensed under the MIT License
 */

#pragma once

#include "ip_address.h"

#include <algorithm>
#include <cstring> //memset
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

//cross platform includes
#ifdef _WIN32 // WINDOWS OS
  // it is written here that for historical reasons, windows.h MUST NOT be included before winsock2.h
  // https://learn.microsoft.com/en-us/windows/win32/winsock/include-files-2
  #include <winsock2.h> // header file containing actual socket function implementations
  #include <ws2tcpip.h> // header file containing various TCP/IP related APIs
  // (converting various data to protocol-compatible format, etc.)
  #pragma comment(lib, "Ws2_32.lib")
  // https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4548?view=msvc-170
  #pragma warning(disable : 4548) // suppress warning that occurs in FD_SET macro on expression "while(0, 0)"
  using socket_t  = SOCKET;
  struct wsa_catcher_t {
    WSADATA wsaData;
    bool    initialized = false;
    int     init_error  = 0;
    wsa_catcher_t (int verbosity = 0) {
      init_error = WSAStartup (MAKEWORD (2, 2), &wsaData);
      if (init_error == 0) {
        initialized = true;
        if (verbosity >= 2)
          std::cout << "WSAStartup() success" << std::endl;
      }
      else {
        initialized = false;
        if (verbosity >= 1)
          std::cerr << "WSAStartup() failed, sockets will NOT work. error: " << init_error << std::endl;
      }
    }
    ~wsa_catcher_t () {
      if (initialized)
        WSACleanup ();
    }
  };
  /// @brief Гарантирует однократную инициализацию Winsock (WSAStartup) на весь процесс.
  /// @param verbosity - уровень логирования: 0 = ничего, 1 = только ошибки, 2 = всё.
  /// @return Ссылка на статический объект wsa_catcher_t (создаётся при первом вызове).
  /// @details WSAStartup/WSACleanup работают по reference counting в Windows, но для
  ///   header-only библиотеки удобнее инициализировать один раз через static local.
  ///   WSACleanup вызовется автоматически при завершении процесса (деструктор static).
  inline wsa_catcher_t& ensure_wsa (int verbosity = 0) {
    static wsa_catcher_t catcher (verbosity);
    return catcher;
  }
#else         // LINUX OS
  // Assume that any non-Windows platform uses POSIX-style sockets instead.
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netdb.h>  // Needed for getaddrinfo() and freeaddrinfo()
  #include <unistd.h> // Needed for close()
  #include <fcntl.h>  // Needed for fcntl() to set non-blocking mode
  #include <poll.h>   // Needed for poll() to implement connect with timeout
  #ifndef closesocket
    #define closesocket(s) ::close(s) // windows uses closesocket(), while linux uses standard close() for descriptors
  #endif
  using socket_t = int;
  #define SOCKET_ERROR   -1
  #define INVALID_SOCKET -1 // since windows returns a pointer, error will be different value "~0" (inverted zero)
#endif

// server:
//  if the server has two ip addresses and is bound to universal ip 0.0.0.0 via bind()
//    then when receiving a packet on its second ip address, the sendto(sender_address) function
//    will trigger sending from the first ip address of the server, since it will be considered the default ip
//    (this will likely only happen if the sender's address is reachable via both server interfaces)

// without connect for client, it's impossible to get port unreachable error
// asynchronous errors are returned to the client only if it explicitly specified remote ip:port via connect()
// Linux returns most ICMP errors about port unreachable even for unconnected sockets, unless the
// SO_DSBCOMPAT socket option is enabled. All errors about unreachable destination shown in table A.5 are returned,
// except errors with codes 0, 1, 4, 5, 11 and 12.

// client:
// if bind() is not bound to specific ip:port
//   then the kernel will choose ip:port
//     but the ip may change with each send depending on the server address (port will always be the same)
// if bind() is bound to ip:port pair
//   then if the kernel chooses some other client ip
//     the message will be sent from the ip chosen by the kernel, but the packet itself will have sender ip specified in bind()

// Information available to the server from incoming IP datagram
//   Client IP datagram      TCP-server   UDP-server
//   Sender IP address       accept       recvfrom
//   Sender port number      accept       recvfrom
//   Receiver IP address     getsockname  recvmsg
//   Receiver port number    getsockname  getsockname

// bind -> setsockname
// connect -> setpeername

// two socket types:
// unconnected socket - need to use sendto / recvfrom commands
// connected socket - need to use sendto(null pointer) send - write / recv - read
//   kernel only forwards packets coming from ip:port pair specified in connect

// connect (AF_UNSPEC) - disconnects UDP socket from ip:port pair

// sending two packets via sendto on unconnected socket:
// socket attachment;
// first datagram output;
// socket detachment;
// socket attachment;
// second datagram output;
// socket detachment.

// sending two packets via write on connected socket:
// socket attachment;
// first datagram output;
// second datagram output;

// up to one third of overhead

// three non-blocking reading options:
// set O_NONBLOCK flag for socket and then catch EWOULDBLOCK error on recvfrom, read, recv calls if no data
// call select (timeout) and check if data is ready for reading. If 0 - no data, if >0 - data exists and then read via recvfrom, read, recv
// set socket option SO_RCVTIMEO = timeout and then read via recvfrom, read, recv and catch EINTR if timeout occurs



//  server:                                                   client:
//  socket()                                                  socket()
//  setsockopt(SO_REUSEADDR)
//  [setsockopt(SO_RCVBUF)]
//  setsockopt(SO_RCVTIMEO)                                   setsockopt(SO_RCVTIMEO)
//  bind()                  [ip_src]:port_src                 connect(->server)        ip_src:port_src ip_dst:port_dst
//  recvfrom(->client)       ip_src:port_src                  sendto(->nullptr)        ip_src:port_src ip_dst:port_dst
//    [sendto(->client)]     ip_src:port_src                  recvfrom(->nullptr)      ip_src:port_src ip_dst:port_dst
//    or
//    connect(->client)      ip_src:port_src ip_dst:port_dst
//    sendto(->nullptr)      ip_src:port_src ip_dst:port_dst
//    recvfrom(->nullptr)    ip_src:port_src ip_dst:port_dst
//    ...
//    connect(->AF_UNSPEC)   ip_src:port_src
//

namespace ipsockets {

  enum class socket_type_e {
    server,
    client
  };

  enum class log_e {
    debug,
    info,
    error,
    none
  };

  enum class state_e {
    created,  ///< socket object created (or was closed); no OS descriptor allocated
    prepared, ///< OS descriptor allocated (socket() called); bind/connect may be in progress or failed
    opened    ///< socket successfully bound (server) or connected (client); ready for send/recv
  };

  enum class udp_type_e {
    dgram,
    raw
  };

  /// @brief Error codes returned by socket operations (recv, send, open, etc.).
  ///   Positive return values from recv/send indicate the number of bytes transferred.
  ///   Zero or negative values indicate errors listed below.
  enum error_e {
    no_error                 =  0, ///< Operation completed successfully
    error_tcp_closed         = -1, ///< TCP connection was closed by the remote peer (recv returned 0)
    error_already_opened     = -2, ///< Attempted to open a socket that is already in opened state
    error_open_failed        = -3, ///< Failed to create OS socket descriptor, or bind/connect/listen failed
    error_closed_or_not_open = -4, ///< Operation attempted on a socket that is not open (e.g. recv before open)
    error_timeout            = -5, ///< Operation timed out (recv SO_RCVTIMEO expired, or connect timeout reached)
    error_unreachable        = -6, ///< Destination unreachable (ECONNREFUSED, WSAECONNRESET, ICMP error)
    error_not_allowed        = -7, ///< Operation not allowed (e.g. recv on server socket, setsockopt failed)
    error_invalid_address    = -8, ///< Invalid remote address specified (EADDRNOTAVAIL)
    error_other              = -9  ///< Unrecognized OS error (original OS error code is logged)
  };

  template <ip_type_e Ip_type>
  struct make_in_addr;

  template <> struct make_in_addr<v4> { using type = in_addr;  };
  template <> struct make_in_addr<v6> { using type = in6_addr; };

  template <ip_type_e Ip_type>
  using make_in_addr_t = typename make_in_addr<Ip_type>::type;

  template <ip_type_e Ip_type>
  struct make_sockaddr_in;

  template <> struct make_sockaddr_in<v4> { using type = sockaddr_in;  };
  template <> struct make_sockaddr_in<v6> { using type = sockaddr_in6; };

  template <ip_type_e Ip_type>
  using make_sockaddr_in_t = typename make_sockaddr_in<Ip_type>::type;

  // forward declaration for friend access from tcp_socket_t
  template <ip_type_e Ip_type, socket_type_e Socket_type>
  struct tcp_socket_t;

  template <ip_type_e Ip_type, socket_type_e Socket_type>
  struct udp_socket_t {

    /// tcp_socket_t needs access to protected members (log_and_return, _getsockname, etc.)
    /// across different Socket_type instantiations (e.g. server's accept() creates client objects)
    template <ip_type_e, socket_type_e> friend struct tcp_socket_t;

    static const ip_type_e     ip_type       = Ip_type;                     ///< IP version: v4 (AF_INET) or v6 (AF_INET6), set by template parameter
    static const socket_type_e socket_type   = Socket_type;                 ///< Socket role: server (binds+listens) or client (connects), set by template parameter
    static const int           af_inet       = (Ip_type == v4) ? AF_INET : AF_INET6; ///< OS address family constant derived from ip_type
    using                      address_t     = addr_t<Ip_type>;             ///< Address type: addr4_t for v4, addr6_t for v6
    using                      in_addr_t     = make_in_addr_t<Ip_type>;     ///< OS in_addr type: in_addr for v4, in6_addr for v6
    using                      sockaddr_in_t = make_sockaddr_in_t<Ip_type>; ///< OS sockaddr type: sockaddr_in for v4, sockaddr_in6 for v6

    state_e  state     = state_e::created; ///< Current socket lifecycle state (created -> prepared -> opened -> created)
    log_e    log_level;                    ///< Logging verbosity: debug (all), info (open/close), error (errors only), none
    socket_t sock      = INVALID_SOCKET;   ///< OS socket descriptor; INVALID_SOCKET when not allocated

    address_t address_local  = {};  ///< Local address:port assigned after open() (by kernel for clients, explicit for servers)
    address_t address_remote = {};  ///< Remote address:port (peer for clients, empty for servers until sendto/recvfrom)

    const int   type;      ///< OS socket type: SOCK_DGRAM (UDP), SOCK_RAW (UDP) or SOCK_STREAM (TCP), set at construction time
    const int   protocol;  ///< OS protocol: IPPROTO_UDP or IPPROTO_TCP, set at construction time
    std::string tname;     ///< Human-readable socket name for log messages, e.g. "udp<ip4,client>" or "tcp<ip6,server>"

    ///	@brief Constructor for UDP socket.
    ///	@param log_level_ - Logging level for this socket instance (default: log_e::info).
    udp_socket_t (log_e log_level_ = log_e::info, udp_type_e udp_type = udp_type_e::dgram)
      : log_level (log_level_), type ((udp_type == udp_type_e::dgram) ? SOCK_DGRAM : SOCK_RAW), protocol (IPPROTO_UDP),
        tname ((udp_type == udp_type_e::dgram) ? _get_tname () : _get_tname_raw ()) {
      #ifdef _WIN32 // WINDOWS OS
      ensure_wsa ((log_level_ == log_e::debug) ? 2 : (log_level_ == log_e::none) ? 0 : 1);
      #endif
    }

    udp_socket_t (udp_socket_t&& os)
      : state (os.state), log_level (os.log_level), sock (os.sock),
        address_local (os.address_local), address_remote (os.address_remote),
        type (os.type), protocol (os.protocol), tname(std::move(os.tname)) {
      os.state          = state_e::created;
      os.sock           = INVALID_SOCKET;
      os.address_local  = {};
      os.address_remote = {};
    }

    udp_socket_t (const udp_socket_t& socket) = delete;

    ~udp_socket_t () {
      close ();
    }

  protected:
    ///	@brief Constructor for derived classes (e.g. tcp_socket_t) to override socket type and protocol.
    udp_socket_t (log_e log_level_, int type_, int protocol_, const std::string& tname_)
      : log_level (log_level_), type (type_), protocol (protocol_), tname (tname_) {
      #ifdef _WIN32 // WINDOWS OS
      ensure_wsa ((log_level_ == log_e::debug) ? 2 : (log_level_ == log_e::none) ? 0 : 1);
      #endif
    }

    /// @brief Returns default type name for logging in static methods (e.g. resolve()).
    static inline std::string _get_tname () {
      return std::string ("udp<") + ((Ip_type == v4) ? "ip4," : "ip6,") + ((Socket_type == socket_type_e::server) ? "server>" : "client>");
    }

    /// @brief Returns type name for RAW mode sockets.
    static inline std::string _get_tname_raw () {
      return std::string ("raw<") + ((Ip_type == v4) ? "ip4," : "ip6,") + ((Socket_type == socket_type_e::server) ? "server>" : "client>");
    }

    address_t _getsockname () {
      sockaddr_in_t addr     = {};
      socklen_t     addr_len = sizeof (sockaddr_in_t);
      getsockname (sock, (sockaddr*)&addr, &addr_len);
      return sockaddr2address (addr);
    }

    address_t _getpeername () {
      sockaddr_in_t addr     = {};
      socklen_t     addr_len = sizeof (sockaddr_in_t);
      getpeername (sock, (sockaddr*)&addr, &addr_len);
      return sockaddr2address (addr);
    }

    static int _get_err () {
      #ifdef _WIN32 // WINDOWS OS
        return WSAGetLastError ();
      #else         // LINUX OS
        return errno;
      #endif
    }

    ///	@brief Performs a non-blocking connect with timeout using poll/WSAPoll.
    ///	@param addr       - Target address in sockaddr_in/sockaddr_in6 format.
    ///	@param timeout_ms - Maximum time to wait for connection in milliseconds.
    ///	@return 0 on success, SOCKET_ERROR on failure (error code available via _get_err()).
    ///	@details Same contract as ::connect() - returns 0/SOCKET_ERROR and sets errno/WSALastError.
    ///	  Internally uses non-blocking connect + poll/WSAPoll to limit the connection wait time.
    ///	  Poll is split into short intervals (500ms) so that close() from another thread
    ///	  is detected promptly via state change to created.
    int _connect (const sockaddr_in_t& addr, uint32_t timeout_ms) {

      // switch socket to non-blocking mode
      #ifdef _WIN32 // WINDOWS OS
        unsigned long nb = 1;
        ioctlsocket (sock, FIONBIO, &nb);
      #else         // LINUX OS
        int flags = fcntl (sock, F_GETFL, 0);
        fcntl (sock, F_SETFL, flags | O_NONBLOCK);
      #endif

      int res = ::connect (sock, (const sockaddr*)&addr, sizeof (sockaddr_in_t));

      if (res == SOCKET_ERROR) {
        int err = _get_err ();
        #ifdef _WIN32 // WINDOWS OS
          bool in_progress = (err == WSAEWOULDBLOCK);
        #else         // LINUX OS
          bool in_progress = (err == EINPROGRESS);
        #endif

        // TCP connect returns EINPROGRESS/WSAEWOULDBLOCK in non-blocking mode, need to poll for completion.
        // UDP connect never returns EINPROGRESS (it is instant), so the compiler will optimize this branch out.
        if (protocol == IPPROTO_TCP && in_progress) {
          // poll in short intervals to allow early exit when close() from another thread sets state to created
          uint32_t remaining = timeout_ms;
          int      rv        = 0;
          while (remaining > 0 && state == state_e::prepared) {
            uint32_t chunk = (remaining > 500) ? 500 : remaining;
            #ifdef _WIN32 // WINDOWS OS
              WSAPOLLFD pfd;  pfd.fd = sock;  pfd.events = POLLWRNORM;
              rv = WSAPoll (&pfd, 1, (int)chunk);
            #else         // LINUX OS
              pollfd pfd;     pfd.fd = sock;  pfd.events = POLLOUT;
              rv = poll (&pfd, 1, (int)chunk);
            #endif
            if (rv != 0) break;
            remaining -= chunk;
          }

          if (state != state_e::prepared)
            err = ECONNABORTED;
          else if (rv > 0) {
            int       so_error = 0;
            socklen_t so_len   = sizeof (so_error);
            getsockopt (sock, SOL_SOCKET, SO_ERROR, (char*)&so_error, &so_len);
            err = so_error;
          }
        }

        if (err == 0)
          res = 0; // connection succeeded
        else {
          // set errno/WSALastError so that _get_err() returns the correct error after this call
          #ifdef _WIN32 // WINDOWS OS
            WSASetLastError ((err == WSAEWOULDBLOCK) ? WSAETIMEDOUT : err);
          #else         // LINUX OS
            errno = (err == EINPROGRESS) ? ETIMEDOUT : err;
          #endif
        }
      }

      // switch socket back to blocking mode (skip if socket was closed from another thread)
      if (state != state_e::created) {
        #ifdef _WIN32 // WINDOWS OS
          nb = 0;
          ioctlsocket (sock, FIONBIO, &nb);
        #else         // LINUX OS
          fcntl (sock, F_SETFL, flags);
        #endif
      }

      return res;
    }

  public:

    ///	@brief Opens a UDP socket and binds it to a local address (server) or connects to a remote address (client).
    ///	@param address            - The address to bind to (for server) or connect to (for client).
    ///	@param timeout_ms         - Receive timeout in milliseconds (SO_RCVTIMEO). Default: 1000.
    ///	@param connect_timeout_ms - Connect timeout in milliseconds (for TCP client sockets only; UDP connect is instant). Default: 5000.
    ///	@return Error code:
    ///	  - no_error on success
    ///	  - error_already_opened if socket is already opened
    ///	  - error_open_failed if socket creation, bind or connect fails
    ///	  - error_timeout if connect timed out
    ///	  - error_not_allowed if setsockopt operations fail
    ///	  - error_other for other errors
    ///	@details For server sockets:
    ///	  - Binds to the specified local address:port
    ///	  - Sets SO_REUSEADDR option to allow multiple listeners
    ///	@details For client sockets:
    ///	  - Connects to the specified remote address:port with timeout via non-blocking connect + poll
    ///	  - Kernel automatically assigns local address:port
    int open (const address_t& address, uint32_t timeout_ms = 1000, uint32_t connect_timeout_ms = 5000) {

      if (state == state_e::opened)     return log_and_return ('-', "open", error_already_opened);

      // open socket
      state = state_e::prepared;
      sock  = socket (af_inet, type, protocol);
      if (sock == INVALID_SOCKET) {
        state = state_e::created;
        log_and_return ('-', "open", _get_err(), (type == SOCK_RAW) ? "socket in RAW mode" : nullptr);
        return error_e::error_open_failed;
      }

      int res;

      if (type == SOCK_RAW) {
          // IP_HDRINCL tells the OS that we provide the IP header ourselves when sending.
          // Works on both Linux and Windows. On Windows requires administrator privileges.
          int ov = 1;
          res    = setsockopt (sock, IPPROTO_IP, IP_HDRINCL, (char*)&ov, sizeof (ov));
          if (res == SOCKET_ERROR) return log_and_return ('-', "setsockopt", _get_err(), "set IP_HDRINCL");
          else                            log_and_return ('-', "setsockopt", no_error,   "set IP_HDRINCL");

          state          = state_e::opened;
          address_remote = address;

          return log_and_return ('-', "open", no_error, "socket in RAW mode");
      }

      // if socket will work in server mode, allow multiple listeners simultaneously
      if (socket_type == socket_type_e::server) {
        int ov = 1;
        res    = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (char*)&ov, sizeof (ov));
        if (res == SOCKET_ERROR) return log_and_return ('-', "setsockopt", _get_err (), "set SO_REUSEADDR");
        else                            log_and_return ('-', "setsockopt", no_error,    "set SO_REUSEADDR");
      }

      // set timeout for recv and recvfrom to periodically "unstick" and allow checking conditions
      #ifdef _WIN32 // WINDOWS OS
        DWORD   tv =   timeout_ms; // in windows this value is stored in DWORD and should be in ms
      #else         // LINUX OS
        timeval tv = { timeout_ms / 1000, timeout_ms % 1000 }; // { sec, msec }
      #endif
      res = setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof (tv));
      if (res == SOCKET_ERROR) return log_and_return ('-', "setsockopt", _get_err(), "set SO_RCVTIMEO");
      else                            log_and_return ('-', "setsockopt", no_error,   "set SO_RCVTIMEO");

      sockaddr_in_t addr = address2sockaddr (address);
      res = (socket_type == socket_type_e::server) ? ::bind (sock, (sockaddr*)&addr, sizeof (sockaddr_in_t)) : _connect  (addr, connect_timeout_ms);
      int err = _get_err ();

      address_local  = (socket_type == socket_type_e::server) ? address     : _getsockname ();
      address_remote = (socket_type == socket_type_e::server) ? address_t{} : address;

      if (res == SOCKET_ERROR) {
        log_and_return ('-', "open", err, (socket_type == socket_type_e::server) ? "bind" : "connect");
        close ();
        return error_open_failed;
      }

      state = state_e::opened;
      return log_and_return ('-', "open", no_error);

    }

    ///	@brief Closes the socket if it has an allocated OS descriptor (state is prepared or opened).
    ///	@return always no_error.
    ///	@details Resets socket state to created and invalidates the socket descriptor.
    ///	  Safe to call from another thread to interrupt a blocking connect() or recv().
    ///	  Safe to call multiple times — subsequent calls are no-ops.
    ///	  Automatically called by destructor.
    int close () {
      if (state != state_e::created) {
        state = state_e::created;
        closesocket (sock);
        sock = INVALID_SOCKET;
        return log_and_return ('-', "close", no_error);
      }
      return no_error;
    }

    ///	@brief Receives data on a connected client socket.
    ///	@param[out] buf     - Buffer to store received data.
    ///	@param      buf_len - Maximum number of bytes to receive.
    ///	@return Number of bytes received on success, or error code:
    ///	  - error_closed_or_not_open if socket not opened
    ///	  - error_not_allowed if called on server socket
    ///	  - error_timeout on receive timeout
    ///	  - error_tcp_closed if connection closed by peer
    ///	  - error_other for other errors
    ///	@pre Socket must be opened in client mode and connected to a remote peer.
    ///	@note Updates address_local and address_remote member variables.
    int recv (char* buf, int buf_len) {

      if (state       != state_e::opened)  return log_and_return ('<', "recv", error_closed_or_not_open);
      if (socket_type == socket_type_e::server) return log_and_return ('<', "recv", error_not_allowed);
      if (type        == SOCK_RAW)         return log_and_return ('<', "recv", error_not_allowed, "use recvfrom() for raw sockets");

      int res        = ::recv (sock, buf, buf_len, 0);
      int err        = _get_err ();
      address_remote = _getpeername();
      address_local  = _getsockname();

      if (res == SOCKET_ERROR) return log_and_return ('<', "recv", err);
      else                     return log_and_return ('<', "recv", no_error, "received", res);
    }

    ///	@brief Receives data on a socket and captures the sender's address.
    ///	@param[out] buf          - Buffer to store received data.
    ///	@param      buf_len      - Maximum number of bytes to receive.
    ///	@param[out] address_from - Output parameter. Filled with the sender's address.
    ///	@return Number of bytes received on success, or error code:
    ///	  - error_closed_or_not_open if socket not opened
    ///	  - error_timeout on receive timeout
    ///	  - error_unreachable if destination unreachable
    ///	  - error_other for other errors
    ///	@details Can be used on both server and client sockets.
    ///	  Updates address_local and address_remote member variables.
    ///	@warning For raw sockets: buffer will contain IP header + UDP header + payload (not just payload).
    ///	  On Linux, kernel filters by protocol (IPPROTO_UDP). On Windows, raw socket receives ALL IP packets
    ///	  on the bound interface - caller must filter by protocol/port manually.
    int recvfrom (char* buf, int buf_len, address_t& address_from) {

      if (state != state_e::opened) return log_and_return ('<', "recvfrom", error_closed_or_not_open);

      socklen_t     addr_len  = sizeof (sockaddr_in_t);
      sockaddr_in_t addr_from = {};
      int           res       = ::recvfrom (sock, buf, buf_len, 0, (sockaddr*)&addr_from, &addr_len);
      int           err       = _get_err ();
      address_remote          = sockaddr2address (addr_from);
      address_local           = _getsockname ();
      address_from            = address_remote;

      if (res == SOCKET_ERROR) return log_and_return ('<', "recvfrom", err);
      else                     return log_and_return ('<', "recvfrom", no_error, "received", res);
    }

    ///	@brief Sends data on a connected client socket.
    ///	@param buf     - Buffer containing data to send.
    ///	@param buf_len - Number of bytes to send.
    ///	@param flags   - Optional flags for send operation (default: 0).
    ///	@return Number of bytes sent on success, or error code:
    ///	  - error_closed_or_not_open if socket not opened
    ///	  - error_not_allowed if called on server socket
    ///	  - error_unreachable if destination unreachable
    ///	  - error_other for other errors
    ///	@pre Socket must be opened in client mode and connected to a remote peer.
    ///	@note Updates address_local and address_remote member variables.
    int send (const char* buf, int buf_len, int flags = 0) {

      if (state       != state_e::opened)  return log_and_return ('>', "send", error_closed_or_not_open);
      if (socket_type == socket_type_e::server) return log_and_return ('>', "send", error_not_allowed);
      if (type        == SOCK_RAW)         return log_and_return ('>', "send", error_not_allowed, "use sendto() for raw sockets");

      int res        = ::send (sock, buf, buf_len, flags);
      int err        = _get_err ();
      address_local  = _getsockname ();
      address_remote = _getpeername ();

      if (res == SOCKET_ERROR) return log_and_return ('>', "send", err);
      else                     return log_and_return ('>', "send", no_error, "sended", res);
    }

    ///	@brief Sends data to a specified destination address.
    ///	@param buf        - Buffer containing data to send.
    ///	@param data_len   - Number of bytes to send.
    ///	@param address_to - Destination address.
    ///	@return Number of bytes sent on success, or error code:
    ///	  - error_closed_or_not_open if socket not opened
    ///	  - error_invalid_address if destination address is invalid
    ///	  - error_unreachable if destination unreachable
    ///	  - error_other for other errors
    ///	@details Can be used on both server and client sockets.
    ///	For raw sockets, extracts source address from IP header.
    ///	Updates address_local and address_remote member variables.
    int sendto (const char* buf, int data_len, address_t& address_to) {

      if (state != state_e::opened) return log_and_return ('>', "sendto", error_closed_or_not_open);

      sockaddr_in_t addr_to = address2sockaddr (address_to);
      int           res     = ::sendto (sock, buf, data_len, 0, (sockaddr*)&addr_to, sizeof (sockaddr_in_t));
      int           err     = _get_err ();
      if (type == SOCK_RAW) {
        const ip4_t*    ip4_ptr     = (const ip4_t*)   (buf + 12);
        const uint16_t* src_port_be = (const uint16_t*)(buf + 20);
        address_local               = addr4_t (*ip4_ptr, orders::ntohT<uint16_t> (*src_port_be));
      }
      else
        address_local       = _getsockname ();
      address_remote        = address_to;

      if (res == SOCKET_ERROR) return log_and_return ('>', "sendto", err);
      else                     return log_and_return ('>', "sendto", no_error, "sended", res);
    }


  protected:

    // processing, output and error code conversion
    int log_and_return (const char dir, const char* func, int err = no_error, const char* mes = 0, int bytes = -1) {

      // convert to our error code
      int cerr;
    
      switch (err) {
        #ifdef _WIN32 // WINDOWS OS
        case WSAETIMEDOUT:     cerr = error_timeout;         break; // windows error code for timeout in recv when SO_RCVTIMEO != 0
        case WSAEADDRNOTAVAIL: cerr = error_invalid_address; break; // error when remote address is invalid
        case WSAECONNREFUSED:
        case WSAECONNRESET:    cerr = error_unreachable;     break; 
        case WSAECONNABORTED:  cerr = error_tcp_closed;      break; 
        #else         // LINUX OS
        case EWOULDBLOCK:      cerr = error_timeout;         break; // linux error code for timeout in recv when SO_RCVTIMEO != 0
        case EADDRNOTAVAIL:    cerr = error_invalid_address; break; // error when remote address is invalid
        case ECONNREFUSED:     cerr = error_unreachable;     break; 
        case EBADF:            cerr = error_closed_or_not_open;        break; // linux Bad file descriptor
        #endif
        default:
          if (err > 0) cerr = error_other;
          else         cerr = err; // if we don't understand type of error and it's error from OS (err<0) we just return error from OS
      }

      // if error is timeout or graceful tcp close, it's not a real error, don't show anything
      // error_timeout    — expected when SO_RCVTIMEO fires, caller should retry or check conditions
      // error_tcp_closed — expected when remote peer closes connection gracefully (recv returns 0)
      if (cerr == error_timeout || cerr == error_tcp_closed)
        return cerr;

      // show message if
      // log_level == debug OR
      // log_level == info AND (function in [open, close, accept] OR error exists) OR
      // log_level == error and error exists
      if (    log_level == log_e::debug
          || (log_level == log_e::info  && (!strcmp (func, "open") || !strcmp (func, "close") || !strcmp (func, "accept") || cerr != no_error))
          || (log_level == log_e::error && cerr != no_error)) {

        // get error description text
        #ifdef _WIN32 // WINDOWS OS
          const char* sysmesl = "";
          LPTSTR sysmes       = NULL;
          DWORD  dwLanguageId = 0x0409; // us-en    for russian leave 0x0000;
          DWORD  dwFlags      = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
          FormatMessageA (dwFlags, NULL, err, dwLanguageId, (LPSTR)&sysmes, 0, NULL);
        #else         // LINUX OS
          const char* sysmesl = "\n";
          const char* sysmes = strerror (errno);
        #endif

        const char* ctext;
        switch (cerr) {
          case no_error:              ctext = "success";                            break;              
          case error_tcp_closed:      ctext = "error, socket closed by other side"; break;
          case error_already_opened:  ctext = "error, socket already opened";       break;
          case error_open_failed:     ctext = "error, failed to open socket";       break;
          case error_closed_or_not_open:        ctext = "error, use closed socket";           break;
          case error_timeout:         ctext = "receive timeout";                    break;
          case error_unreachable:     ctext = "unreachable error";                  break;
          case error_not_allowed:     ctext = "error, not allowed on this mode";    break;
          case error_invalid_address: ctext = "error address";                      break;
          default:                    ctext = "error";                              break;
        }
      
        std::stringstream result;
      
        result << tname << ": ";
      
        if (sock == INVALID_SOCKET) result << "[undefined]";
        else                        result << '[' << std::hex << sock << std::dec << ']';
      
        result << '.' << func << "() ";
      
        //if (address_local.port != 0 || address_remote.port != 0) {
          std::string local  = (address_local.port  != 0) ? address_local.to_str ()  : "undefined";
          std::string remote = (address_remote.port != 0) ? address_remote.to_str () : "undefined";
          if      (dir == '>') result << "[" << local << " -> " << remote << "] ";
          else if (dir == '<') result << "[" << local << " <- " << remote << "] ";
          else                 result << "[" << local << " <> " << remote << "] ";
        //}
      
        if (mes) result << mes << ' ';
      
        if      (bytes >= 0) result << bytes << " bytes\n";
        else if (err > 0)    result << ctext << ", system answer: " << sysmes << sysmesl;
        else                 result << ctext << std::endl;
      
        std::cout << result.str();
      
        #ifdef _WIN32 // WINDOWS OS
          LocalFree (sysmes);
        #endif

      }

      if (bytes >= 0) return bytes;
      else            return cerr;

    // udp<ip4,server>: wsa initialisation error
    // tname + ": [sock]." + func + "() [" + addr_src + " -> " + addr_dst + "] " + cust_mes + [B bytes] + text(cerr) + [sys_answer(err)]/"\n"

    // tname + ": [sock]." + func + "() [" + addr_src + " -> " + addr_dst + "] " + cust_mes + B bytes"\n"
    // tname + ": [sock]." + func + "() [" + addr_src + " -> " + addr_dst + "] " + cust_mes + B bytes"\n"
    // tname + ": [sock]." + func + "() [" + addr_src + " -> " + addr_dst + "] " + cust_mes + [B bytes] + text(cerr) + [sys_answer(err)]/"\n"
    // tname + ": [sock]." + func + "() [" + addr_src + " -> " + addr_dst + "] " + cust_mes + [B bytes] + text(cerr) + [sys_answer(err)]/"\n"
    // udp<ip4,server>: [0x0002].open() [192.168.1.1:2000 -> "unspecified"] bind success
    // udp<ip4,server>: [0x0002].open() [192.168.1.1:2000 -> "unspecified"] error, already opened
    // udp<ip4,server>: [0x0002].open() [192.168.1.1:2000 -> "unspecified"] error, wsa initialisation error
    // udp<ip4,server>: [0x0002].open() [192.168.1.1:2000 -> "unspecified"] socket error, system answer: bla bla bla
    // udp<ip4,server>: [0x0002].open() [192.168.1.1:2000 -> "unspecified"] bind error, system answer: bla bla bla

    // udp<ip4,client>: [0x0002].open() ["unspecified"    -> 192.168.1.1:2000] connect success
    // udp<ip4,client>: [0x0002].open() [192.168.1.1:2000 -> "unspecified"   ] connect error, system answer: bla bla bla

    // udp<ip4,server>: [0x0002].recv() ["unspecified"    -> 192.168.1.1:2001] error, not allowed on server mode
    // udp<ip4,server>: [0x0002].send() [192.168.1.1:2001 -> "unspecified"   ] error, not allowed on server mode
    // udp<ip4,client>: [0x0002].recv() [192.168.1.1:2001 -> 192.168.1.1:2001] received 9 bytes
    // udp<ip4,client>: [0x0002].recv() [192.168.1.1:2001 -> 192.168.1.1:2001] error, system answer: bla bla bla
    // udp<ip4,client>: [0x0002].send() [192.168.1.1:2001 -> 192.168.1.1:2001] sended 7777 bytes
    // udp<ip4,client>: [0x0002].send() [192.168.1.1:2001 -> 192.168.1.1:2001] error, system answer: bla bla bla
    // udp<ip4,client>: [0x0002].send() [192.168.1.1:2001 -> 192.168.1.1:2001] unreachable error, system answer: bla bla bla

    // udp<ip4,client>: [0x0002].recvfrom() [192.168.1.1:2001 -> 192.168.1.1:2000] received 9 bytes
    // udp<ip4,client>: [0x0002].sendto()   [192.168.1.1:2000 -> 192.168.1.1:2001] sended 7777 bytes
    // udp<ip4,client>: [0x0002].sendto()   [192.168.1.1:2000 -> 192.168.1.1:2001] error, system answer: bla bla bla

    }

  public:

    // static address conversion functions

    /// @brief Converts addr4_t (ip4 + port) to OS sockaddr_in structure for use with socket API calls.
    /// @param address - Source address in addr4_t format (host byte order port, network byte order ip).
    /// @return sockaddr_in structure with AF_INET family, network byte order port and ip address.
    static inline sockaddr_in address2sockaddr (const addr4_t& address) {
      sockaddr_in result;
      memset (&result, 0, sizeof (result));
      result.sin_family = af_inet;
      result.sin_port   = orders::htonT (address.port);
      result.sin_addr   = *((in_addr*)&address.ip);
      return result;
    }

    /// @brief Converts addr6_t (ip6 + port) to OS sockaddr_in6 structure for use with socket API calls.
    /// @param address - Source address in addr6_t format (host byte order port, network byte order ip).
    /// @return sockaddr_in6 structure with AF_INET6 family, network byte order port and ip address.
    static inline sockaddr_in6 address2sockaddr (const addr6_t& address) {
      sockaddr_in6 result;
      memset (&result, 0, sizeof (result));
      result.sin6_family = af_inet;
      result.sin6_port   = orders::htonT (address.port);
      result.sin6_addr   = *((in6_addr*)&address.ip);
      return result;
    }

    /// @brief Converts OS sockaddr_in structure to addr4_t (ip4 + port).
    /// @param address - Source address in sockaddr_in format (network byte order).
    /// @return addr4_t with network byte order ip and host byte order port.
    /// @throws std::exception if address family is AF_INET6 (wrong overload called).
    static inline addr4_t sockaddr2address (const sockaddr_in& address) {
      addr4_t result;
      if (address.sin_family == AF_INET6) throw;
      *((in_addr*)&result.ip) = address.sin_addr;
      result.port             = orders::ntohT (address.sin_port);
      return result;
    }

    /// @brief Converts OS sockaddr_in6 structure to addr6_t (ip6 + port).
    /// @param address - Source address in sockaddr_in6 format (network byte order).
    /// @return addr6_t with network byte order ip and host byte order port.
    /// @throws std::exception if address family is AF_INET (wrong overload called).
    static inline addr6_t sockaddr2address (const sockaddr_in6& address) {
      addr6_t result;
      if (address.sin6_family == AF_INET) throw;
      *((in6_addr*)&result.ip) = address.sin6_addr;
      result.port              = orders::ntohT (address.sin6_port);
      return result;
    }

    ///	@brief Resolves a hostname to an IP address using DNS.
    ///	@param      hostname  - Hostname to resolve (e.g., "example.com").
    ///	@param[out] success   - Optional output flag. If non-null, set to true on successful resolution, false on failure.
    ///	@param      log_level - Logging level for DNS resolution messages (default: log_e::error).
    ///	@return ip_t<Ip_type> Resolved IP address. Returns empty IP (all zeros) on failure.
    ///	@details Performs DNS lookup using getaddrinfo():
    ///	  - For IPv4 sockets (Ip_type = v4) resolves to IPv4 addresses only
    ///	  - For IPv6 sockets (Ip_type = v6) resolves to IPv6 addresses only
    ///	  - Returns the first matching address found in DNS response
    static inline ip_t<Ip_type> resolve (const std::string& hostname, bool* success = nullptr, log_e log_level = log_e::error) {
      return _resolve (hostname, success, log_level, _get_tname());
    }

  protected:

    static inline ip_t<Ip_type> _resolve (const std::string& hostname, bool* success, log_e log_level, const std::string& tname) {

      ip_t<Ip_type> result = {};
      int           family = Ip_type == v4 ? AF_INET : AF_INET6;
      bool found = false;

      #ifdef _WIN32 // WINDOWS OS
        int verbosity = (log_level == log_e::debug) ? 2 : (log_level == log_e::none) ? 0 : 1;
        wsa_catcher_t& wsa = ensure_wsa (verbosity);
        if (!wsa.initialized)
          return result;
      #endif

      struct addrinfo hints, *res = nullptr;
      std::memset(&hints, 0, sizeof(hints));
      hints.ai_family   = family;         // IPv4 or IPv6
      hints.ai_socktype = SOCK_STREAM;    // TCP (any socktype works for getaddrinfo)
      hints.ai_flags    = AI_NUMERICSERV; // service not needed

      int err = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);

      if (err == 0) {
        for (struct addrinfo* rp = res; rp != nullptr; rp = rp->ai_next) {
          found = check_and_copy(rp, result);
          if (found)
            break; // successfully copied IP address, exit loop
        }
        freeaddrinfo(res);
      }

      if (log_level <= log_e::info || (log_level == log_e::error && (err != 0 || found == false))) {

        std::stringstream log_message;

        log_message << tname << ": [static].resolve()   [undefined -> " << hostname << "] ";
        if (err != 0)            log_message << "DNS resolution failed: " << gai_strerror(err) << std::endl;
        else if (found == false) log_message << "DNS resolution succes, but address with " << (Ip_type == v4 ? "IPv4" : "IPv6") << " type not found in DNS answer" << std::endl;
        else                     log_message << "DNS resolution success, resolved to '" << result.to_str() << "'" << std::endl;

        std::cout << log_message.str();

      }

      if (success)
        *success = (err == 0 && found);

      return result;
    }

  private:

    static inline bool check_and_copy (struct addrinfo* rp, ip4_t& ip) {
      if (rp->ai_family == AF_INET) {
        struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(rp->ai_addr);
        *((in_addr*)&ip) = sin->sin_addr;
        return true;
      }
      return false;
    }

    static inline bool check_and_copy (struct addrinfo* rp, ip6_t& ip) {
      if (rp->ai_family == AF_INET6) {
        struct sockaddr_in6* sin6 = reinterpret_cast<struct sockaddr_in6*>(rp->ai_addr);
        *((in6_addr*)&ip) = sin6->sin6_addr;
        return true;
      }
      return false;
    }
  };

} // namespace ipsockets
