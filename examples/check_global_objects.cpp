/*
* prometheus-cpp-lite — header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* check_global_objects.cpp — Global objects usage example
*
* Demonstrates using pre-defined global objects from prometheus-cpp-lite-full:
* global_registry, file_saver, http_server, http_pusher.
* All metric types are created with the global registry and exposed via
* all three export modes simultaneously (HTTP pull, HTTP push, file).
*
*/

#include <prometheus/prometheus.h>

using namespace prometheus;

int main () {

  counter_metric_t   requests  ("http_requests_total",      "Total requests");
  gauge_metric_t     active    ("active_connections",       "Open connections");
  histogram_metric_t latency   ("request_duration_seconds", "Request latency");
  summary_metric_t   response  ("response_time_seconds",    "Response time");
  benchmark_metric_t uptime    ("uptime_seconds",           "Process uptime");

  file_saver. start(std::chrono::seconds(5), "./metrics.txt");
  http_server.start("127.0.0.1:9091");
  http_pusher.start(std::chrono::seconds(5), "http://localhost:9091/metrics/job/test");

  uptime.start();
  for (int i = 0; i < 60; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    requests++;
    active.Set       (10    +  std::rand() % 50);
    latency.Observe  (0.001 * (std::rand() % 1000));
    response.Observe (0.001 * (std::rand() % 500));
    if (i % 10 == 0)
      std::cout << global_registry.serialize() << std::endl;
  }
  uptime.stop();
}
