/*
* prometheus-cpp-lite — header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* quick_start.cpp — Quick start example
*
* Demonstrates all metric types (counter, gauge, histogram, summary, benchmark)
* exposed via HTTP pull server at http://localhost:9100/metrics.
* This is the code from the "All metric types at a glance" section in README.
*
*/

#include <prometheus/prometheus.h>

namespace prometheus { registry_t global_registry; }

using namespace prometheus;

int main() {
  registry_t         registry;

  counter_metric_t   requests  (registry, "http_requests_total",      "Total requests");
  gauge_metric_t     active    (registry, "active_connections",       "Open connections");
  histogram_metric_t latency   (registry, "request_duration_seconds", "Request latency");
  summary_metric_t   response  (registry, "response_time_seconds",    "Response time");
  benchmark_metric_t uptime    (registry, "uptime_seconds",           "Process uptime");

  http_server_t      server    (registry, {{127,0,0,1}, 9100});
  
  // curl http://localhost:9100/metrics

  uptime.start();
  for (int i = 0; i < 60; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    requests++;
    active.Set(10 + std::rand() % 50);
    latency.Observe(0.001 * (std::rand() % 1000));
    response.Observe(0.001 * (std::rand() % 500));
  }
  uptime.stop();
}
