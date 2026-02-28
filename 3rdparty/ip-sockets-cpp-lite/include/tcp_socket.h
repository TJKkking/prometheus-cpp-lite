
/*
 * ip-sockets-cpp-lite — header-only C++ networking utilities
 * https://github.com/biaks/ip-sockets-cpp-lite
 * 
 * Copyright (c) 2021 Yan Kryukov ianiskr@gmail.com
 * Licensed under the MIT License
 */

#pragma once

#include "udp_socket.h"

namespace ipsockets {

  template <ip_type_e Ip_type, socket_type_e Socket_type>
  struct tcp_socket_t : udp_socket_t<Ip_type, Socket_type> {

    using base_socket_t = udp_socket_t<Ip_type, Socket_type>;
    using typename base_socket_t::address_t;

    tcp_socket_t<Ip_type, socket_type_e::server>* parent = nullptr;
    std::vector<socket_t>                         accept_clients;

    ///	@brief Constructor for TCP socket.
    ///	@param log_level - Logging level for this socket instance (default: log_e::info).
    tcp_socket_t (log_e log_level = log_e::info)
      : base_socket_t (log_level) {
      this->type     = SOCK_STREAM;
      this->protocol = IPPROTO_TCP;
      this->tname    = std::string ("tcp<") + ((this->ip_type == v4) ? "ip4," : "ip6,")
                     + ((this->socket_type == socket_type_e::server) ? "server>" : "client>");
    }

    tcp_socket_t (const tcp_socket_t& socket) = delete;

    tcp_socket_t (tcp_socket_t&& other_socket) : base_socket_t (std::move(other_socket)) {
      parent               = other_socket.parent;
      other_socket.parent  = nullptr;
      accept_clients       = std::move (other_socket.accept_clients);
    }

    ~tcp_socket_t () {
      if (parent == nullptr)
        for (socket_t& accept_sock : accept_clients)
          closesocket (accept_sock);
      close ();
    }

    ///	@brief Opens a TCP socket and performs binding/connection.
    ///	@param address            - The address to bind to (for server) or connect to (for client).
    ///	@param timeout_ms         - Receive timeout in milliseconds (default: 1000).
    ///	@param max_incoming_queue - Maximum length of the pending connections queue for server sockets (default: 1000).
    ///	@return Error code:
    ///	  - no_error on success
    ///	  - error_not_allowed if called on an accepted client socket
    ///	  - error_already_opened if socket is already opened
    ///	  - error_open_failed if socket creation or listen fails
    ///	  - error_other for other errors
    ///	@details For server sockets:
    ///	  - Calls base class open() to create and bind the socket
    ///	  - Calls listen() to mark socket as passive and accept incoming connections
    ///	@details For client sockets:
    ///	  - Simply calls base class open() to create and connect the socket
    int open (const address_t& address, uint32_t timeout_ms = 1000, int max_incoming_queue = 1000) {

      if ( parent != nullptr ) return this->log_and_return ('<', "open", error_not_allowed);

      int result = base_socket_t::open (address, timeout_ms);

      if (this->socket_type == socket_type_e::server) {
        int res = listen (this->sock, max_incoming_queue); 

        if (res == SOCKET_ERROR) {
          this->log_and_return ('-', "open", this->_get_err (), "listen");
          closesocket (this->sock);
          return error_open_failed;
        }
      }

      return result;

    }

    ///	@brief Receives data on a connected TCP socket.
    ///	@param[out] buf     - Buffer to store received data.
    ///	@param      buf_len - Maximum number of bytes to receive.
    ///	@return Number of bytes received on success, or error code:
    ///	  - error_tcp_closed if connection was closed by peer (recv returned 0)
    ///	  - error_closed_or_not_open if socket not opened
    ///	  - error_not_allowed if called on server socket
    ///	  - error_timeout on receive timeout
    ///	  - error_other for other errors
    ///	@pre Socket must be opened in client mode and connected to a remote peer.
    int recv (char* buf, int buf_len) {

      // udp socket can receive a zero-length datagram,
      // tcp socket CANNOT receive data of 0 bytes, the value 0 indicates that
      // the connection was intentionally terminated by the other side
      // for linux   base_socket_t::recv received 0 and then we correct it to error_tcp_closed
      // for windows base_socket_t::recv received error_tcp_closed because it catch WSAECONNABORTED error code from OS
      int result = base_socket_t::recv (buf, buf_len);
      return (result == 0)
        ? this->log_and_return ('<', "recv", error_tcp_closed)
        : result;

    }

    ///	@brief Closes the TCP socket and removes it from parent's accepted connections list.
    ///	@return Error code or no_error on successful close.
    ///	@note Accepted sockets will be automatically closed when the server object is destroyed.
    int close () {
      if (this->state == state_e::state_opened && parent)
        for (size_t i = 0; i < parent->accept_clients.size (); i++)
          if (parent->accept_clients[i] == this->sock)
            parent->accept_clients.erase (parent->accept_clients.begin () + i);
      return base_socket_t::close ();
    }

    ///	@brief Waits for and accepts an incoming TCP connection on a listening server socket.
    ///	@param[out] address_from - Filled with the remote peer address of the accepted connection. On failure it is set to an empty address.
    ///	@param[out] success      - Optional output flag. If non-null, set to true on successful accept, or false on failure.
    ///	@return tcp_socket_t<Ip_type, socket_type_e::client> A client socket representing the accepted connection.
    ///	  On failure a default-constructed client socket is returned (with state == state_e::state_created).
    ///	@pre The method must be called on an opened socket of type server. If the socket is not opened
    ///	 or not a server, the function returns immediately with a default client socket and logs an error.
    ///	@details Platform-specific behavior:
    ///	  - On Linux: Uses SO_RCVTIMEO socket option for timeout handling
    ///	  - On Windows: Uses WSAPoll() with timeout from SO_RCVTIMEO (or default 1000ms) to simulate accept timeout
    ///	@note The accepted socket is automatically added to parent's accepted connections list
    ///	  and will have its parent pointer set to this server socket.
    template <socket_type_e SOCK = Socket_type, std::enable_if_t<SOCK == socket_type_e::server, bool> = true>
    tcp_socket_t<Ip_type, socket_type_e::client> accept (address_t& address_from, bool* success = nullptr) {

      using sockaddr_in_t = typename base_socket_t::sockaddr_in_t;

      tcp_socket_t<Ip_type, socket_type_e::client> result (this->log_level);

      int cerr = no_error;

      if (this->state       != state_e::state_opened) cerr = error_closed_or_not_open;
      if (this->socket_type != socket_type_e::server) cerr = error_not_allowed;

      if (cerr != no_error) {
        this->log_and_return ('-', "accept", cerr);
        return result;
      }

      socklen_t     addr_len  = sizeof (sockaddr_in_t);
      sockaddr_in_t addr_from = {};

      #ifdef _WIN32 // WINDOWS OS
        // for windows ::accept will never be timeout and we should close sock connection to interrupt it or we should using poll()
        DWORD tv_ms  = 1000; // get SO_RCVTIMEO if set (DWORD on Windows), fallback to 1000 ms
        int   optlen = sizeof(tv_ms);
        if (getsockopt(this->sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv_ms, &optlen) == SOCKET_ERROR)
          tv_ms = 1000;

        WSAPOLLFD pfd;
        pfd.fd     = this->sock;
        pfd.events = POLLRDNORM; // wait incoming data

        int rv = WSAPoll(&pfd, 1, (int)tv_ms);
        if (rv == 0) { // timeout
          result.sock = INVALID_SOCKET;
          cerr        = error_timeout;
        }
        else if (pfd.revents & POLLRDNORM) { // data ready
          result.sock = ::accept (this->sock, (sockaddr*)&addr_from, &addr_len);
          cerr        = this->_get_err ();
        }
        else { // error
          result.sock = INVALID_SOCKET;
          cerr        = this->_get_err ();
        }
      #else         // LINUX OS
        // for linux   ::accept will be timeout by value SO_RCVTIMEO wich we set by setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof (tv));
        result.sock = ::accept (this->sock, (sockaddr*)&addr_from, &addr_len);
        cerr = this->_get_err ();
      #endif

      if (result.sock == INVALID_SOCKET) {
        this->log_and_return ('-', "accept", cerr);
        address_from = {};
        if (success)
          *success = false;
        return result;
      }

      address_from          = this->sockaddr2address (addr_from);

      result.address_remote = address_from;
      result.address_local  = result._getsockname ();
      result.state          = state_e::state_opened;
      //result.log_level      = this->log_level; set via constructor
      result.parent         = this;
      result.tname          = std::string ("tcp<") + ((this->ip_type == v4) ? "ip4," : "ip6,") + "accept>";

      result.log_and_return ('-', "accept", no_error);
      accept_clients.push_back (result.sock);
      if (success)
        *success = true;
      return result;

    }

  };

} // namespace ipsockets
