
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

#include <thread>
#include <chrono>
#include <string>
#include <sstream>
#include <future>

namespace prometheus {

  using namespace std::chrono_literals;
  using namespace ipsockets;

  /// @brief HTTP client that pushes metrics to a Prometheus Pushgateway.
  ///
  /// Supports two usage modes:
  ///
  /// **Periodic mode** — a background thread pushes metrics at a configurable interval:
  /// @code
  ///   http_pusher_t pusher(registry, std::chrono::seconds(10),
  ///                        "http://pushgateway:9091/metrics/job/myapp");
  /// @endcode
  ///
  /// **On-demand mode** — compatible with prometheus-cpp Gateway API:
  /// @code
  ///   http_pusher_t gw("pushgateway", "9091", "myapp", {{"instance", "host1"}});
  ///   gw.RegisterCollectable(registry);
  ///   gw.Push();      // POST  — replace all metrics for this job
  ///   gw.PushAdd();   // PUT   — update only these metrics
  ///   gw.Delete();    // DELETE — remove metrics for this job
  /// @endcode
  class http_pusher_t {

    using tcp_client_t = tcp_socket_t<v4, socket_type_e::client>;

    std::shared_ptr<registry_t> registry_ptr  { nullptr };
    std::thread                 worker_thread;
    std::atomic<bool>           must_die      { false };

    std::chrono::seconds        period        { 10 };
    std::string                 server_host   { "localhost" };
    ip4_t                       server_ip     { "127.0.0.1" };
    uint16_t                    server_port   { 9091 };
    std::string                 server_path   { "/" };

    /// @brief HTTP method for push operations.
    enum class http_method_e : uint8_t {
      http_post,
      http_put,
      http_delete,
    };

  public:

    // =========================================================================
    // Construction / Destruction
    // =========================================================================

    /// @brief Default constructor.  Registry, period, and URI must be set before calling start().
    http_pusher_t() = default;

    /// @brief Constructs with a shared registry.  Call start() to begin pushing.
    /// @param registry_ Shared pointer to the registry to push.
    explicit http_pusher_t(std::shared_ptr<registry_t> registry_)
      : registry_ptr(std::move(registry_)) {}


    /// @brief Constructs with a shared registry.  Call start() to begin saving.
    /// @param registry_ Registry to serialize.
    explicit http_pusher_t(registry_t& registry_)
      : registry_ptr(make_non_owning(registry_)) {}

    /// @brief Constructs with a shared registry, push period, and target URI.  Starts immediately.
    /// @param registry_ Shared pointer to the registry to push.
    /// @param period_   Interval between pushes.
    /// @param uri_      Full URI of the Pushgateway endpoint (e.g. "http://host:9091/metrics/job/myapp").
    http_pusher_t(std::shared_ptr<registry_t> registry_, const std::chrono::seconds& period_, const std::string& uri_)
      : registry_ptr(std::move(registry_)), period(period_) {
      set_uri(uri_);
      start();
    }

    /// @brief Constructs with a registry reference, push period, and target URI.  Starts immediately.
    /// @param registry_ Registry to push.
    /// @param period_   Interval between pushes.
    /// @param uri_      Full URI of the Pushgateway endpoint.
    /// @note The registry must outlive the pusher.
    http_pusher_t(registry_t& registry_, const std::chrono::seconds& period_, const std::string& uri_)
      : registry_ptr(make_non_owning(registry_)), period(period_) {
      set_uri(uri_);
      start();
    }

    /// @brief Gateway-compatible constructor: builds the URI from host, port, job name, and labels.
    ///
    /// Does NOT start a background thread.  Use Push(), PushAdd(), or Delete()
    /// for on-demand operation, or call start() for periodic pushing.
    ///
    /// @param host    Pushgateway hostname or IP (without scheme).
    /// @param port    Pushgateway port as a string.
    /// @param jobname Job name for the Pushgateway grouping key.
    /// @param labels  Additional grouping labels (e.g. {{"instance", "host1"}}).
    http_pusher_t(const std::string& host, const std::string& port, const std::string& jobname, const Labels& labels = {})
      : server_host(host), server_port(static_cast<uint16_t>(std::stoi(port))) {
      // Build path: /metrics/job/<jobname>[/label1/value1/...]
      std::ostringstream path_stream;
      path_stream << "/metrics/job/" << jobname;
      for (const auto& label : labels)
        path_stream << "/" << label.first << "/" << label.second;
      server_path = path_stream.str();
      server_ip   = ip4_t(server_host);
    }

    /// @brief Stops the pusher and joins the worker thread.
    ~http_pusher_t() {
      stop();
    }

    // Non-copyable (owns a thread).
    http_pusher_t(const http_pusher_t&) = delete;
    http_pusher_t& operator=(const http_pusher_t&) = delete;

    // =========================================================================
    // Configuration
    // =========================================================================

    /// @brief Replaces the registry to push.
    /// @param registry_ New shared pointer to a registry.
    void set_registry(std::shared_ptr<registry_t> registry_) {
      registry_ptr = std::move(registry_);
    }

    /// @brief Sets the push interval.
    /// @param period_ Interval between pushes.
    void set_delay(const std::chrono::seconds& period_) {
      period = period_;
    }

    /// @brief Parses and sets the target URI.
    ///
    /// The URI must include a scheme (http://), a host, and optionally a port
    /// and path.  Example: "http://pushgateway:9091/metrics/job/myapp".
    ///
    /// @param uri Full URI string.
    /// @throws std::invalid_argument if the host cannot be extracted from the URI.
    void set_uri(const std::string& uri) {
      parse_uri(uri, server_host, server_port, server_path);
      if (server_host.empty())
        throw std::invalid_argument("Host is required in URI");
      server_ip = ip4_t(server_host);
    }

    // =========================================================================
    // Gateway-compatible API (on-demand, synchronous)
    // =========================================================================

    /// @brief Registers a registry for pushing.
    ///
    /// Gateway-compatible method.  In the original prometheus-cpp Gateway this
    /// accepted a weak_ptr<Collectable>; here it accepts a shared_ptr<registry_t>.
    ///
    /// @param registry Shared pointer to the registry to push.
    void RegisterCollectable(std::shared_ptr<registry_t> registry) {
      registry_ptr = std::move(registry);
    }

    /// @brief Returns an instance label map for the given hostname.
    ///
    /// Gateway-compatible static helper.
    ///
    /// @param hostname Instance hostname (empty string returns empty labels).
    /// @return Labels map with a single "instance" key, or empty if hostname is empty.
    static Labels GetInstanceLabel(const std::string& hostname) {
      if (hostname.empty())
        return {};
      return {{"instance", hostname}};
    }

    /// @brief Pushes metrics via HTTP POST (replaces all metrics for this job on the Pushgateway).
    /// @return HTTP status code, or -1 on connection failure.
    int Push() {
      return perform_request(http_method_e::http_post);
    }

    /// @brief Pushes metrics via HTTP PUT (updates only the sent metrics, preserves others).
    /// @return HTTP status code, or -1 on connection failure.
    int PushAdd() {
      return perform_request(http_method_e::http_put);
    }

    /// @brief Deletes metrics for this job from the Pushgateway via HTTP DELETE.
    /// @return HTTP status code, or -1 on connection failure.
    int Delete() {
      return perform_request(http_method_e::http_delete);
    }

    /// @brief Asynchronous version of Push().
    /// @return Future with the HTTP status code.
    std::future<int> AsyncPush() {
      return std::async(std::launch::async, &http_pusher_t::Push, this);
    }

    /// @brief Asynchronous version of PushAdd().
    /// @return Future with the HTTP status code.
    std::future<int> AsyncPushAdd() {
      return std::async(std::launch::async, &http_pusher_t::PushAdd, this);
    }

    /// @brief Asynchronous version of Delete().
    /// @return Future with the HTTP status code.
    std::future<int> AsyncDelete() {
      return std::async(std::launch::async, &http_pusher_t::Delete, this);
    }

    // =========================================================================
    // Periodic push control
    // =========================================================================

    /// @brief Sets the push period and URI, then starts the worker thread.
    /// @param period_ Interval between pushes.
    /// @param uri_    Full URI of the Pushgateway endpoint.
    void start(const std::chrono::seconds& period_, const std::string& uri_) {
      set_delay(period_);
      set_uri(uri_);
      start();
    }

    /// @brief Starts the pusher worker thread.
    void start() {
      must_die = false;
      worker_thread = std::thread{ &http_pusher_t::worker_function, this };
    }

    /// @brief Stops the pusher and joins the worker thread.
    void stop() {
      must_die = true;
      if (worker_thread.joinable())
        worker_thread.join();
    }

  private:

    // =========================================================================
    // HTTP request execution
    // =========================================================================

    /// @brief Returns the HTTP method string for the given enum value.
    static const char* method_string(http_method_e method) {
      switch (method) {
        case http_method_e::http_post:   return "POST";
        case http_method_e::http_put:    return "PUT";
        case http_method_e::http_delete: return "DELETE";
        default:                         return "POST";
      }
    }

    /// @brief Resolves the server hostname to an IP address if not already resolved.
    /// @return true if the IP is valid after the call.
    bool ensure_resolved() {
      if (server_ip)
        return true;
      server_ip = tcp_client_t::resolve(server_host);
      if (server_ip)
        std::cout << "http_pusher_t: resolved host " << server_host << " to ip " << server_ip.to_str() << std::endl;
      else
        std::cout << "http_pusher_t: failed to resolve host " << server_host << std::endl;
      return static_cast<bool>(server_ip);
    }

    /// @brief Performs an HTTP request with the given method.
    ///
    /// For POST and PUT, serializes the registry as the request body.
    /// For DELETE, sends an empty body.
    ///
    /// @param method HTTP method to use.
    /// @return HTTP status code (e.g. 200, 202), or -1 on connection/network failure.
    int perform_request(http_method_e method) {
      if (!ensure_resolved())
        return -1;

      tcp_client_t sock;
      if (sock.open(addr4_t{server_ip, server_port}) != no_error)
        return -1;

      // Serialize the body (empty for DELETE).
      std::string body;
      if (method != http_method_e::http_delete && registry_ptr) {
        std::ostringstream body_stream;
        registry_ptr->serialize(body_stream);
        body = body_stream.str();
      }

      // Build the HTTP request.
      std::ostringstream request_stream;
      request_stream << method_string(method) << " " << server_path << " HTTP/1.1\r\n";
      request_stream << "Host: " << server_host << ":" << server_port << "\r\n";
      request_stream << "Content-Type: text/plain; version=0.0.4; charset=utf-8\r\n";
      request_stream << "Content-Length: " << body.size() << "\r\n";
      request_stream << "Connection: close\r\n";
      request_stream << "\r\n";
      if (!body.empty())
        request_stream << body;
      std::string request = request_stream.str();

      sock.send(request.c_str(), static_cast<int>(request.size()));

      // Read the response and extract the status code.
      char buf[1024];
      int  res = sock.recv(buf, static_cast<int>(sizeof(buf) - 1));
      sock.close();

      if (res <= 0)
        return -1;

      return parse_http_status(buf, res);
    }

    /// @brief Extracts the HTTP status code from a raw response buffer.
    ///
    /// Expects the response to start with "HTTP/1.x NNN ...".
    ///
    /// @param buf  Raw response bytes.
    /// @param len  Number of valid bytes.
    /// @return HTTP status code, or -1 if parsing fails.
    static int parse_http_status(const char* buf, int len) {
      // Minimum: "HTTP/1.1 200" = 12 chars
      if (len < 12)
        return -1;

      // Find the space after "HTTP/1.x"
      const char* space = static_cast<const char*>(memchr(buf, ' ', static_cast<size_t>(len)));
      if (!space || space - buf + 4 > len)
        return -1;

      char* endptr = nullptr;
      long  code   = std::strtol(space + 1, &endptr, 10);
      if (endptr == space + 1 || code < 100 || code > 599)
        return -1;

      return static_cast<int>(code);
    }

    // =========================================================================
    // Worker (periodic mode)
    // =========================================================================

    /// @brief Worker loop: periodically serializes the registry and POSTs it to the Pushgateway.
    void worker_function() {
      const uint64_t divider  = 100;
      uint64_t       fraction = divider;

      while (true) {
        std::chrono::milliseconds period_ms = std::chrono::duration_cast<std::chrono::milliseconds>(period);
        std::this_thread::sleep_for(period_ms / divider);

        if (must_die || --fraction == 0) {
          fraction = divider;

          if (registry_ptr)
            perform_request(http_method_e::http_post);

          if (must_die)
            return;
        }
      }
    }

    // =========================================================================
    // URI parsing
    // =========================================================================

    /// @brief Parses an HTTP URI into host, port, and path components.
    static bool parse_uri(const std::string& uri, std::string& out_host, uint16_t& out_port, std::string& out_path) {

      out_port = 80;

      std::size_t scheme_pos = uri.find("://");
      if (scheme_pos == std::string::npos)
        return false;

      std::size_t host_start = scheme_pos + 3;
      if (host_start >= uri.length())
        return false;

      bool        is_ipv6_literal = false;
      std::size_t host_end;
      std::size_t port_start      = std::string::npos;

      if (uri[host_start] == '[') {
        is_ipv6_literal = true;
        std::size_t close_bracket = uri.find(']', host_start);
        if (close_bracket == std::string::npos)
          return false;

        host_end = close_bracket;
        if (close_bracket + 1 < uri.length() && uri[close_bracket + 1] == ':')
          port_start = close_bracket + 2;

      }
      else {
        host_end = host_start;
        while (host_end < uri.length() && uri[host_end] != ':' && uri[host_end] != '/')
          ++host_end;

        if (host_end < uri.length() && uri[host_end] == ':')
          port_start = host_end + 1;
      }

      out_host = uri.substr(host_start, host_end - host_start);
      if (is_ipv6_literal && out_host.length() >= 2)
        out_host = out_host.substr(1, out_host.length() - 2);

      if (port_start != std::string::npos) {
        std::size_t port_end = port_start;
        while (port_end < uri.length() && uri[port_end] != '/')
          ++port_end;
        std::string port_str = uri.substr(port_start, port_end - port_start);
        if (!port_str.empty()) {
          char* endptr = nullptr;
          long  p      = std::strtol(port_str.c_str(), &endptr, 10);
          if (endptr != port_str.c_str() && p > 0 && p <= 65535)
            out_port = static_cast<uint16_t>(p);
          else
            return false;
        }
      }

      std::size_t path_start = uri.find('/', host_start);
      out_path =  (path_start == std::string::npos) ? "/" :  uri.substr(path_start);

      return true;
    }
  };

  /// @brief Alias for backward compatibility with the previous periodic push API.
  using PushToServer = http_pusher_t;

  /// @brief Alias for prometheus-cpp Gateway compatibility.
  using Gateway = http_pusher_t;

} // namespace prometheus
