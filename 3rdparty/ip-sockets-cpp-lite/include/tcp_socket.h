
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

  // ============================================================
  // tcp_socket_t — TCP socket implementation
  // ============================================================

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

  // ============================================================
  // tcp_streambuf_t — std::streambuf adapter for tcp_socket_t
  // ============================================================

  template <ip_type_e Ip_type, socket_type_e Socket_type>
  struct tcp_streambuf_t : public std::streambuf {

    using tcp_t     = tcp_socket_t<Ip_type, Socket_type>;
    using address_t = addr_t<Ip_type>;

    tcp_t             sock;
    std::vector<char> input_buffer;
    std::vector<char> output_buffer;

    int last_read_error  = no_error;
    int last_write_error = no_error;

    static const std::size_t default_bufsize = 8192;

    /// @brief Constructs a streambuf from an already-opened TCP socket.
    /// @param sock_   - TCP socket (moved into the streambuf).
    /// @param bufsize - Size of internal read/write buffers (default: 8192).
    tcp_streambuf_t (tcp_t&& sock_, std::size_t bufsize = default_bufsize)
      : sock (std::move (sock_)), input_buffer (bufsize), output_buffer (bufsize) {
      setg (input_buffer.data (), input_buffer.data (), input_buffer.data ());     // set get area to empty (will be filled on first read)
      setp (output_buffer.data (), output_buffer.data () + output_buffer.size ()); // set put area to full buffer
    }

    tcp_streambuf_t (const tcp_streambuf_t&) = delete;
    tcp_streambuf_t& operator= (const tcp_streambuf_t&) = delete;

    tcp_streambuf_t (tcp_streambuf_t&& other)
      : std::streambuf   (std::move (other)),
      sock             (std::move (other.sock)),
      input_buffer     (std::move (other.input_buffer)),
      output_buffer    (std::move (other.output_buffer)),
      last_read_error  (other.last_read_error),
      last_write_error (other.last_write_error) {
      // recalculate get area pointers relative to new buffer
      // after move, the old pointers are invalid, reset to empty
      setg (input_buffer.data (), input_buffer.data (), input_buffer.data ());     // set get area to empty (will be filled on first read)
      setp (output_buffer.data (), output_buffer.data () + output_buffer.size ()); // set put area to full buffer
    }

    ~tcp_streambuf_t () {
      flush ();
    }

    /// @brief Sends all buffered output data through the socket.
    /// @return true on success, false on error.
    bool flush () {
      std::ptrdiff_t size = pptr () - pbase ();
      if (size > 0) {
        int n = sock.send (pbase (), (int)size);
        if (n != (int)size) {
          last_write_error = n;
          return false;
        }
        last_write_error = no_error;
      }
      setp (output_buffer.data (), output_buffer.data () + output_buffer.size ()); // reset put area to full buffer
      return true;
    }

    state_e   state ()          const { return sock.state; }          ///< @brief Returns the current socket state.
    address_t remote_address () const { return sock.address_remote; } ///< @brief Returns the remote address of the socket.
    address_t local_address ()  const { return sock.address_local; }  ///< @brief Returns the local address of the socket.

  protected:

    // called when the write buffer is full or eof is written
    int_type overflow (int_type ch) override {
      if (ch != traits_type::eof ()) {
        if (pptr () == epptr () && !flush ()) // if buffer is full, flush it before writing new char
          return traits_type::eof ();         // flush failed, return eof to indicate error
        *pptr () = traits_type::to_char_type (ch);
        pbump (1);
      }     
      else if (!flush ())                     // if ch is eof, just flush the buffer
        return traits_type::eof ();           // flush failed, return eof to indicate error
      return traits_type::not_eof (ch);
    }

    // called when the read buffer is exhausted
    int_type underflow () override {
      if (gptr () < egptr ())
        return traits_type::to_int_type (*gptr ());

      int n = sock.recv (input_buffer.data (), (int)input_buffer.size ());
      if (n <= 0) {
        last_read_error = n;
        return traits_type::eof ();
      }
      last_read_error = no_error;

      setg (input_buffer.data (), input_buffer.data (), input_buffer.data () + n); // set get area to the newly read data
      return traits_type::to_int_type (*gptr ());
    }

    // called on std::flush / std::endl
    int sync () override {
      return flush () ? 0 : -1;
    }

  };

  // ============================================================
  // tcp_stream_t — std::iostream over a TCP socket
  // ============================================================

  template <ip_type_e Ip_type, socket_type_e Socket_type>
  struct tcp_stream_t : public std::iostream {

    using tcp_t       = tcp_socket_t<Ip_type, Socket_type>;
    using address_t   = addr_t<Ip_type>;
    using streambuf_t = tcp_streambuf_t<Ip_type, Socket_type>;

    streambuf_t buf;

    /// @brief Constructs a tcp_stream from an already-opened TCP socket.
    /// @param socket  - TCP socket (moved into the stream).
    /// @param bufsize - Size of internal read/write buffers (default: 8192).
    /// @details After construction the stream is ready for reading and writing
    ///   using standard iostream operators (<< and >>), std::getline, etc.
    tcp_stream_t (tcp_t&& socket, std::size_t bufsize = streambuf_t::default_bufsize)
      : std::iostream (&buf), buf (std::move (socket), bufsize) {}

    tcp_stream_t (const tcp_stream_t&) = delete;
    tcp_stream_t& operator= (const tcp_stream_t&) = delete;

    tcp_stream_t (tcp_stream_t&& other)
      : std::iostream (&buf), buf (std::move (other.buf)) {
      // re-bind the iostream to the new buffer after move
      rdbuf (&buf);
    }

    state_e      state ()            const { return buf.state (); }          ///< @brief Returns the current socket state.
    address_t    remote_address ()   const { return buf.remote_address (); } ///< @brief Returns the remote address.
    address_t    local_address ()    const { return buf.local_address (); }  ///< @brief Returns the local address.
    int          last_read_error ()  const { return buf.last_read_error; }   ///< @brief Returns last read error code.
    int          last_write_error () const { return buf.last_write_error; }  ///< @brief Returns last write error code.
    tcp_t&       socket ()                 { return buf.sock; }              ///< @brief Returns a reference to the underlying TCP socket for advanced operations.
    const tcp_t& socket ()           const { return buf.sock; }              ///< @brief Returns a const reference to the underlying TCP socket for advanced operations.

  };

} // namespace ipsockets
