/*
* prometheus-cpp-lite — header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*/

#pragma once

#include "prometheus/core.h"

#include "tcp_socket.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <thread>
#include <string>
#include <future>
#include <list>
#include <vector>
#include <sstream>
#include <unordered_map>

namespace prometheus {

  using namespace std::chrono_literals;
  using namespace ipsockets;

  /// @brief Simple HTTP server that exposes Prometheus metrics via pull endpoints.
  ///
  /// Supports two usage modes:
  ///
  /// **Simple mode** — a single registry is exposed at the default `/metrics` path:
  /// @code
  ///   auto registry = std::make_shared<registry_t>();
  ///   http_server_t server(registry, {127,0,0,1}, 9100);
  ///   // metrics available at http://localhost:9100/metrics
  /// @endcode
  ///
  /// **Multi-path mode** — multiple registries are registered under different paths:
  /// @code
  ///   http_server_t server;
  ///   auto app_registry  = std::make_shared<registry_t>();
  ///   auto sys_registry  = std::make_shared<registry_t>();
  ///   server.add_endpoint(app_registry, "/metrics/app");
  ///   server.add_endpoint(sys_registry, "/metrics/sys");
  ///   server.start({127,0,0,1}, 9100);
  ///   // app metrics  at http://localhost:9100/metrics/app
  ///   // sys metrics  at http://localhost:9100/metrics/sys
  /// @endcode
  class http_server_t {

    using tcp_server_t = tcp_socket_t<v4, socket_type_e::server>;
    using tcp_client_t = tcp_socket_t<v4, socket_type_e::client>;
    using endpoints_t  = std::unordered_map<std::string, std::shared_ptr<registry_t>>;

    endpoints_t        endpoints;        /// @brief Map from URL path to a shared registry.
    mutable std::mutex endpoints_mutex;  ///< Protects the endpoints map.

    std::thread        worker_thread;
    std::atomic<bool>  must_die       { false };

    addr4_t            server_address { "0.0.0.0", 9100 };
    tcp_server_t       server_socket  { log_e::error };

  public:

    // =========================================================================
    // Construction / Destruction
    // =========================================================================

    /// @brief Default constructor.
    ///
    /// No endpoints are registered.  Use add_endpoint() to register paths,
    /// then call start() to begin serving.
    http_server_t(log_e log_level = log_e::error) : server_socket(log_level) {}

    /// @brief No endpoints are registered, starts immediately.
    ///
    /// Use add_endpoint() to register paths.
    /// @param server_address_ Address and port to listen on.
    /// @param log_level       Socket logging level (default: log_e::error).
    http_server_t(addr4_t server_address_, log_e log_level = log_e::error)
      : server_address(server_address_), server_socket(log_level) {
      start();
    }

    /// @brief Constructs with a shared registry.  Call start() to begin saving.
    /// @param registry_  Registry to serialize.
    /// @param log_level  Socket logging level (default: log_e::error).
    explicit http_server_t(registry_t& registry_, log_e log_level = log_e::error)
      : server_socket(log_level) {
      add_endpoint(make_non_owning(registry_), "/metrics");
    }

    /// @brief Simple-mode constructor: single registry at a custom path, starts immediately.
    /// @param server_address_ Address and port to listen on.
    /// @param registry_       Shared pointer to the registry to expose.
    /// @param path_           URL path to serve metrics at (default: "/metrics").
    /// @param log_level       Socket logging level (default: log_e::error).
    http_server_t(addr4_t server_address_, std::shared_ptr<registry_t> registry_, const std::string& path_ = "/metrics", log_e log_level = log_e::error)
      : server_address(server_address_), server_socket(log_level) {
      add_endpoint(std::move(registry_), path_);
      start();
    }

    /// @brief Simple-mode constructor: single registry at a custom path, starts immediately.
    /// @param registry_       Shared pointer to the registry to expose.
    /// @param server_address_ Address and port to listen on.
    /// @param path_           URL path to serve metrics at (default: "/metrics").
    /// @param log_level       Socket logging level (default: log_e::error).
    http_server_t(std::shared_ptr<registry_t> registry_, addr4_t server_address_, const std::string& path_ = "/metrics", log_e log_level = log_e::error)
      : server_address(server_address_), server_socket(log_level) {
      add_endpoint(std::move(registry_), path_);
      start();
    }

    /// @brief Simple-mode constructor: single registry at a custom path, starts immediately.
    /// @param registry_       registry to expose.
    /// @param server_address_ Address and port to listen on.
    /// @param path_           URL path to serve metrics at (default: "/metrics").
    /// @param log_level       Socket logging level (default: log_e::error).
    http_server_t(registry_t& registry_, addr4_t server_address_, const std::string& path_ = "/metrics", log_e log_level = log_e::error)
      : server_address(server_address_), server_socket(log_level) {
      add_endpoint(make_non_owning(registry_), path_);
      start();
    }

    /// @brief Stops the server and joins the worker thread.
    ~http_server_t() {
      stop();
    }

    // Non-copyable, non-movable (owns a thread).
    http_server_t(const http_server_t&) = delete;
    http_server_t& operator=(const http_server_t&) = delete;

    // =========================================================================
    // Configuration
    // =========================================================================

    /// @brief Sets the socket logging level.
    ///
    /// Affects the server socket and all subsequently accepted client sockets.
    /// Does not affect already accepted connections.
    ///
    /// @param level New logging level.
    void set_log_level(log_e level) {
      server_socket.log_level = level;
    }

    // =========================================================================
    // Endpoint management
    // =========================================================================

    /// @brief Registers a registry under the given URL path.
    ///
    /// If a registry is already registered at this path, it is replaced.
    /// Can be called while the server is running — the endpoints map is
    /// protected by a mutex.
    ///
    /// @param registry Shared pointer to the registry to expose at this path.
    /// @param path     URL path (e.g. "/metrics", "/metrics/app").
    void add_endpoint(std::shared_ptr<registry_t> registry, const std::string& path) {
      std::lock_guard<std::mutex> lock(endpoints_mutex);
      endpoints[path] = std::move(registry);
    }

    /// @brief Registers a registry under the given URL path.
    ///
    /// If a registry is already registered at this path, it is replaced.
    /// Can be called while the server is running — the endpoints map is
    /// protected by a mutex.
    ///
    /// @param registry The registry to expose at this path.
    /// @param path     URL path (e.g. "/metrics", "/metrics/app").
    void add_endpoint(registry_t& registry, const std::string& path) {
      std::lock_guard<std::mutex> lock(endpoints_mutex);
      endpoints[path] = make_non_owning(registry);
    }

    /// @brief Removes the endpoint at the given URL path.
    ///
    /// Does nothing if no registry is registered at this path.
    ///
    /// @param path URL path to remove.
    void remove_endpoint(const std::string& path) {
      std::lock_guard<std::mutex> lock(endpoints_mutex);
      endpoints.erase(path);
    }

    /// @brief Replaces the registry at the default `/metrics` path.
    ///
    /// Convenience method for simple-mode usage.
    ///
    /// @param registry New shared pointer to a registry.
    /// @param path_    URL path (default: "/metrics").
    void set_registry(std::shared_ptr<registry_t> registry, const std::string& path_ = "/metrics") {
      add_endpoint(std::move(registry), path_);
    }

    /// @brief Replaces the registry at the default `/metrics` path.
    ///
    /// Convenience method for simple-mode usage.
    ///
    /// @param registry The registry.
    /// @param path_    URL path (default: "/metrics").
    void set_registry(registry_t& registry, const std::string& path_ = "/metrics") {
      add_endpoint(make_non_owning(registry), path_);
    }

    /// @brief Registers a collectable (registry) at the default `/metrics` path and starts the server.
    /// @param registry Shared pointer to the registry to expose.
    void RegisterCollectable(std::shared_ptr<registry_t> registry) {
      add_endpoint(std::move(registry), "/metrics");
    }

    // =========================================================================
    // Server control
    // =========================================================================

    /// @brief Sets the server listen address (does not restart automatically).
    /// @param server_address_ New address and port.
    void set_server_address(addr4_t server_address_) {
      server_address = server_address_;
    }

    /// @brief Sets the server address and starts listening.
    /// @param server_address_ Address and port to listen on.
    void start(addr4_t server_address_, log_e log_level = log_e::error) {
      set_server_address(server_address_);
      set_log_level(log_level);
      start();
    }

    /// @brief Starts the HTTP server worker thread.
    void start() {
      // Validate listen address.
      if (server_address.port == 0)
        throw std::runtime_error("http_server_t::start(): server port is 0 — call set_server_address() with a valid port before start()");
      // Stop the previous thread if it is still running
      stop();
      must_die      = false;
      worker_thread = std::thread{ &http_server_t::worker_function, this };
    }

    /// @brief Stops the HTTP server and joins the worker thread.
    void stop() {
      must_die = true;
      server_socket.close();
      if (worker_thread.joinable())
        worker_thread.join();
    }

  private:

    // =========================================================================
    // Worker
    // =========================================================================

    /// @brief Main worker loop: opens the server socket and accepts clients.
    void worker_function() {

      while (true) {
        if (server_socket.open(server_address) == no_error) {
          addr4_t accepted_client_addr;

          // List of tasks for handling clients.
          std::list<std::future<void>> accepted_clients_futures;

          // Main loop for accepting new clients.
          while (server_socket.state == state_e::state_opened) {
            tcp_client_t accepted_client = server_socket.accept(accepted_client_addr);
            if (accepted_client.state == state_e::state_opened) {
              accepted_clients_futures.emplace_back(
                std::async(std::launch::async, &http_server_t::accepted_client_func, this, std::move(accepted_client)));

              // Prune completed tasks.
              for (std::list<std::future<void>>::iterator it = accepted_clients_futures.begin(); it != accepted_clients_futures.end();) {
                if (it->wait_for(0s) == std::future_status::ready)
                  it = accepted_clients_futures.erase(it);
                else
                  ++it;
              }
            }
          }

          // Wait for all remaining tasks.
          for (std::future<void>& f : accepted_clients_futures)
            f.wait();
        }

        if (must_die)
          return;

        // Retry after a short delay, checking must_die frequently.
        for (int i = 0; i < 10 && !must_die; ++i)
          std::this_thread::sleep_for(1s);

        if (must_die)
          return;
      }
    }

    // =========================================================================
    // Request handling
    // =========================================================================

    /// @brief Extracts the request path from an HTTP GET request line.
    ///
    /// Expects the buffer to start with "GET /path HTTP/...".
    /// Returns the path portion, or an empty string if parsing fails.
    ///
    /// @param buffer Raw request bytes.
    /// @param length Number of valid bytes in the buffer.
    /// @return Extracted path string, or empty on failure.
    static std::string extract_request_path(const char* buffer, int length) {
      // Minimum valid: "GET / HTTP/1.0\r\n" — at least 16 chars.
      if (length < 5 || strncmp(buffer, "GET ", 4) != 0)
        return {};

      const char* path_begin = buffer + 4;
      const char* path_end   = static_cast<const char*>(memchr(path_begin, ' ', static_cast<size_t>(length - 4)));

      if (!path_end)
        return {};

      // Strip query string if present (e.g. "/metrics?format=text").
      std::string path(path_begin, path_end);
      std::string::size_type qpos = path.find('?');
      if (qpos != std::string::npos)
        path.resize(qpos);

      return path;
    }

    /// @brief Looks up a registry for the given request path.
    /// @param path URL path from the request.
    /// @return Shared pointer to the matching registry, or nullptr if not found.
    std::shared_ptr<registry_t> find_registry(const std::string& path) const {
      std::lock_guard<std::mutex> lock(endpoints_mutex);
      endpoints_t::const_iterator it = endpoints.find(path);
      if (it != endpoints.end())
        return it->second;
      return nullptr;
    }

    /// @brief Handles a single accepted client connection.
    ///
    /// Reads the HTTP request.  If the path matches a registered endpoint,
    /// serializes the corresponding registry and sends it back with
    /// appropriate HTTP headers.  If the path is "/" and no endpoint is
    /// registered there, returns a simple HTML index page with links to
    /// all registered endpoints.  Otherwise returns a 404 response.
    ///
    /// @param accepted_client The accepted TCP client socket (moved in).
    void accepted_client_func(tcp_client_t accepted_client) {

      const size_t      BUFFER_SIZE = 8192;
      std::vector<char> buffer(BUFFER_SIZE, 0);

      int res = accepted_client.recv(buffer.data(), static_cast<int>(BUFFER_SIZE - 1));
      if (res <= 0)
        return;

      std::string path = extract_request_path(buffer.data(), res);
      if (path.empty()) {
        if (server_socket.log_level <= log_e::error)
          std::cout << "http_server_t: 400 Bad Request (could not parse request path)" << std::endl;
        send_404(accepted_client);
        return;
      }

      // Check for a registered endpoint first.
      std::shared_ptr<registry_t> registry = find_registry(path);
      if (registry) {
        if (server_socket.log_level <= log_e::info)
          std::cout << "http_server_t: 200 GET " << path << std::endl;
        send_metrics(accepted_client, *registry);
        return;
      }

      // If the root path is requested and not registered, show an index page.
      if (path == "/") {
        if (server_socket.log_level <= log_e::info)
          std::cout << "http_server_t: 200 GET / (index page)" << std::endl;
        send_index(accepted_client);
        return;
      }

      if (server_socket.log_level <= log_e::error)
        std::cout << "http_server_t: 404 GET " << path << " (no endpoint registered)" << std::endl;
      send_404(accepted_client);
    }

    /// @brief Serializes a registry and sends it as an HTTP 200 response.
    /// @param client   TCP client socket.
    /// @param registry Registry to serialize.
    void send_metrics(tcp_client_t& client, registry_t& registry) {
      std::ostringstream metrics_stream;
      registry.serialize(metrics_stream);
      std::string metrics = metrics_stream.str();

      std::ostringstream headers_stream;
      time_t now = time(nullptr);
      headers_stream << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: text/plain; version=0.0.4; charset=utf-8\r\n"
                     << "Content-Length: " << metrics.size() << "\r\n"
                     << "Connection: close\r\n"
                     << "Date: " << std::put_time(gmtime(&now), "%a, %d %b %Y %H:%M:%S %Z") << "\r\n"
                     << "\r\n";

      std::string headers = headers_stream.str();
      client.send(headers.data(), static_cast<int>(headers.size()));
      if (!metrics.empty())
        client.send(metrics.data(), static_cast<int>(metrics.size()));
    }

    /// @brief Sends a simple HTML index page listing all registered endpoints.
    /// @param client TCP client socket.
    void send_index(tcp_client_t& client) {
      std::ostringstream body_stream;
      body_stream << "<html>\n"
                       "<head>\n"
                         "<title>Prometheus Metrics Exporter</title>\n"
                       "</head>\n"
                       "<body>\n"
                         "<h1>Prometheus Metrics Exporter</h1>\n"
                         "<ul>\n";
      {
        std::lock_guard<std::mutex> lock(endpoints_mutex);
        if (endpoints.empty())
          body_stream << "<li><em>No endpoints registered</em></li>\n";
        else {
          // Sort endpoints alphabetically for stable output.
          std::map<std::string, std::shared_ptr<registry_t>> sorted(endpoints.begin(), endpoints.end());
          for (const auto& entry : sorted)
            body_stream << "<li><a href=\"" << entry.first << "\">" << entry.first << "</a></li>\n";
        }
      }
      body_stream <<     "</ul>\n"
                       "</body>\n"
                     "</html>\n";
      std::string body = body_stream.str();

      std::ostringstream headers_stream;
      time_t now = time(nullptr);
      headers_stream << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: text/html; charset=utf-8\r\n"
                     << "Content-Length: " << body.size() << "\r\n"
                     << "Connection: close\r\n"
                     << "Date: " << std::put_time(gmtime(&now), "%a, %d %b %Y %H:%M:%S %Z") << "\r\n"
                     << "\r\n";

      std::string headers = headers_stream.str();
      client.send(headers.data(), static_cast<int>(headers.size()));
      client.send(body.data(),    static_cast<int>(body.size()));
    }

    /// @brief Sends a 404 Not Found response.
    /// @param client TCP client socket.
    static void send_404(tcp_client_t& client) {
      const char* response = "HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
      client.send(response, static_cast<int>(strlen(response)));
    }
  };

  using Exposer = http_server_t;

} // namespace prometheus
