/*
* prometheus-cpp-lite — header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* use_metrics_in_class_advanced.cpp — Dynamic per-instance metrics with labels
*
* Demonstrates a real-world pattern: a class that manages multiple named
* connections (e.g. to upstream servers), where each connection has its own
* set of metrics distinguished by labels.
*
* Uses prometheus-cpp-lite-full target which provides pre-defined global objects.
*
* Key concept: families are NOT stored anywhere — each time we create a metric
* with a new label set, we simply construct a family_t with the same name.
* Because global_registry deduplicates families by name, the same underlying
* Family object is reused.  This means:
*
*   - No need to pass family references between classes.
*   - No need to store families as class members.
*   - Just create a metric anywhere, using the same family name — it will
*     automatically join the existing family in global_registry.
*
* This pattern is common in routers, proxies, connection pools, and any
* system where the set of monitored entities is not known at compile time.
*/

#include <prometheus/prometheus.h>

#include <vector>
#include <memory>

using namespace prometheus;

// =============================================================================
// Connection — represents a single upstream connection with its own metrics
//
// Each instance creates metrics with unique label values {name, protocol}.
// Families are found (or created) by name in global_registry every time —
// no need to store or pass family references.
// =============================================================================

class Connection {
  std::string        name_;
  std::string        protocol_;

  // Per-instance metrics.
  counter_metric_t   bytes_sent;
  counter_metric_t   bytes_recv;
  counter_metric_t   errors;
  gauge_metric_t     is_connected;
  histogram_metric_t latency;

public:

  Connection(const std::string& name, const std::string& protocol)
    : name_        (name)
    , protocol_    (protocol)
    , bytes_sent   (counter_metric_t   ("upstream_bytes_sent_total", "Bytes sent to upstream",           {{"name", name}, {"protocol", protocol}}))
    , bytes_recv   (counter_metric_t   ("upstream_bytes_recv_total", "Bytes received from upstream",     {{"name", name}, {"protocol", protocol}}))
    , errors       (counter_metric_t   ("upstream_errors_total",     "Upstream connection errors",       {{"name", name}, {"protocol", protocol}}))
    , is_connected (gauge_metric_t     ("upstream_connected",        "Connection status (1=up, 0=down)", {{"name", name}, {"protocol", protocol}}))
    , latency      (histogram_metric_t ("upstream_latency_seconds",  "Upstream request latency",         {{"name", name}, {"protocol", protocol}},
                    BucketBoundaries{0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1.0})) {
    is_connected = 1;
    std::cout << "    [+] Connection created: " << name << " (" << protocol << ")\n";
  }

  void send(int bytes) {
    double latency_sec = 0.001 * (1 + std::rand() % 100);
    bytes_sent += bytes;
    latency.Observe(latency_sec);

    // Simulate occasional errors.
    if (std::rand() % 20 == 0) {
      errors++;
      is_connected = 0;
    }
  }

  void receive(int bytes) {
    double latency_sec = 0.001 * (1 + std::rand() % 50);
    bytes_recv += bytes;
    latency.Observe(latency_sec);
  }

  void reconnect() {
    is_connected = 1;
  }

  const std::string& name() const { return name_; }
};

// =============================================================================
// ConnectionPool — manages a dynamic set of connections
//
// No families stored here.  Each Connection finds/creates its own families
// by name in global_registry.
// =============================================================================

class ConnectionPool {

  std::vector<std::unique_ptr<Connection>> connections;

public:

  ConnectionPool() = default;

  /// @brief Adds a new connection — metrics are created dynamically in global_registry.
  Connection& add_connection (const std::string& name, const std::string& protocol) {
    connections.push_back(std::make_unique<Connection>(name, protocol));
    return *connections.back();
  }

  /// @brief Simulates traffic on all connections.
  void simulate_traffic () {
    for (std::unique_ptr<Connection>& conn : connections) {
      conn->send  (100 + std::rand() % 900);
      conn->receive(50 + std::rand() % 500);

      // Occasionally reconnect downed connections.
      if (std::rand() % 5 == 0)
        conn->reconnect();
    }
  }

  size_t size() const { return connections.size(); }
};

// =============================================================================
// main
// =============================================================================

int main() {
  std::cout << "=== Advanced class instrumentation — dynamic per-instance metrics ===\n";
  std::cout << "  Metrics available at http://localhost:9100/metrics\n\n";

  // One line — all metrics from all classes are exposed.
  http_server.start("127.0.0.1:9100");

  ConnectionPool pool;

  // Phase 1: Start with two connections.
  std::cout << "  Phase 1: Creating initial connections...\n";
  pool.add_connection("gateway-eu-west", "tcp");
  pool.add_connection("gateway-us-east", "tcp");

  for (int i = 0; i < 10; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pool.simulate_traffic();
  }
  std::cout << "\n" << global_registry.serialize() << std::endl;

  // Phase 2: Add more connections dynamically (e.g. new upstream discovered).
  std::cout << "  Phase 2: Adding more connections dynamically...\n";
  pool.add_connection("gateway-ap-south", "tcp");
  pool.add_connection("cache-local",      "udp");

  for (int i = 0; i < 10; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pool.simulate_traffic();
  }
  std::cout << "\n" << global_registry.serialize() << std::endl;

  // Phase 3: Continue running — all 4 connections producing metrics.
  std::cout << "  Phase 3: Running with " << pool.size() << " connections for 20 seconds...\n";
  for (int i = 0; i < 40; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pool.simulate_traffic();

    if (i % 10 == 9)
      std::cout << "  tick " << (i + 1) << "/40\n";
  }

  std::cout << "\n  Final metrics:\n" << global_registry.serialize() << std::endl;
  std::cout << "  Done.\n";

  return 0;
}