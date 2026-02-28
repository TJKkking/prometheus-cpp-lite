/*
* prometheus-cpp-lite — header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* provide_via_http_pull_advanced.cpp — HTTP pull endpoint examples
*
* Demonstrates two ways to expose metrics via http_server_t:
*
*   example_simple()     — Single registry at /metrics (most common case).
*   example_multipath()  — Multiple registries at different URL paths.
*
* Both examples run sequentially for 10 seconds each.
* Open a browser or use curl to see the output:
*
*   Simple mode:
*     curl http://localhost:9100/metrics
*
*   Multi-path mode:
*     curl http://localhost:9200/metrics/app
*     curl http://localhost:9200/metrics/sys
*/

#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/http_puller.h>

using namespace prometheus;

// =============================================================================
// Example 1 — Simple mode
//
// The most common scenario: one registry, one endpoint at /metrics.
// Three lines of setup, then just update your metrics in a loop.
//
//   curl http://localhost:9100/metrics
// =============================================================================

void example_simple() {
  std::cout << "\n=== Example 1: Simple mode — single registry at /metrics ===\n";
  std::cout << "  Listening on http://localhost:9100/metrics for 10 seconds...\n\n";

  // 1. Create a shared registry.
  std::shared_ptr<registry_t> registry = std::make_shared<registry_t>();

  // 2. Create metrics inside it.
  counter_metric_t requests (registry, "http_requests_total", "Total HTTP requests");
  gauge_metric_t   active   (registry, "active_connections",  "Currently open connections");

  // 3. Start the HTTP server — metrics are now available at http://localhost:9100/metrics
  http_server_t server({{127,0,0,1}, 9100}, registry);

  // Simulate application work.
  for (int i = 0; i < 10; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    int rnd = std::rand() % 10;
    requests += rnd;
    active.Set(5 + std::rand() % 20);

    std::cout << "  [simple] tick " << (i + 1) << "/10"
      << "  requests=" << requests.Get()
      << "  active="   <<   active.Get() << std::endl;
  }

  // Server stops automatically when it goes out of scope.
  std::cout << "  Stopped.\n";
}

// =============================================================================
// Example 2 — Multi-path mode
//
// Useful when you want to separate application metrics from system/internal
// metrics, or expose metrics from different modules at different endpoints.
//
//   curl http://localhost:9200/metrics/app
//   curl http://localhost:9200/metrics/sys
// =============================================================================

void example_multipath() {
  std::cout << "\n=== Example 2: Multi-path mode — multiple registries at different paths ===\n"
    << "  Listening on http://localhost:9200 for 10 seconds...\n"
    << "    /metrics/app  — application metrics\n"
    << "    /metrics/sys  — system metrics\n\n";

  // 1. Create separate registries for different metric domains.
  std::shared_ptr<registry_t> app_registry = std::make_shared<registry_t>();
  std::shared_ptr<registry_t> sys_registry = std::make_shared<registry_t>();

  // 2. Populate each registry with its own metrics.
  counter_metric_t orders_total   (app_registry, "orders_total",   "Total orders processed");
  gauge_metric_t   queue_size     (app_registry, "queue_size",     "Current queue depth");

  gauge_metric_t   cpu_usage      (sys_registry, "cpu_usage_percent", "CPU usage percentage");
  gauge_metric_t   memory_used_mb (sys_registry, "memory_used_mb",    "Memory usage in MB");

  // 3. Create the server and register each registry under its own path.
  http_server_t server;
  server.add_endpoint(app_registry, "/metrics/app");
  server.add_endpoint(sys_registry, "/metrics/sys");
  server.start({{127,0,0,1}, 9200});

  // Simulate application work.
  for (int i = 0; i < 10; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    orders_total += 1 + std::rand() % 5;
    queue_size.Set(std::rand() % 50);

    cpu_usage.Set(10 + std::rand() % 80);
    memory_used_mb.Set(200 + std::rand() % 300);

    std::cout << "  [multi]  tick " << (i + 1) << "/10"
      << "  orders="  << orders_total.Get()
      << "  queue="   << queue_size.Get()
      << "  cpu="     << cpu_usage.Get() << "%"
      << "  mem="     << memory_used_mb.Get() << "MB" << std::endl;
  }

  // Server stops automatically when it goes out of scope.
  std::cout << "  Stopped.\n";
}

// =============================================================================
// main
// =============================================================================

int main() {
  std::cout << "=== HTTP Pull endpoint examples ===\n";
  std::cout << "Use curl or a browser to fetch metrics while the examples run.\n";

  example_simple();
  example_multipath();

  return 0;
}