/*
* prometheus-cpp-lite — header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* use_metrics_in_class_simple.cpp — Using metrics inside a class
*
* Demonstrates the simplest way to add Prometheus metrics to an existing class.
* Metrics are declared as class members and incremented in business methods.
*
* Uses prometheus-cpp-lite-full target which provides pre-defined global objects:
*   global_registry, http_server, http_pusher, file_saver.
*
* All metrics created with the two-argument constructor (name, help)
* automatically register in global_registry.
*/

#include <prometheus/prometheus.h>

using namespace prometheus;

// =============================================================================
// MyService — a simple class instrumented with Prometheus metrics
// =============================================================================

class MyService {

  // Metrics as class members — registered in global_registry automatically.
  counter_metric_t requests_total  {"myservice_requests_total",  "Total requests processed"};
  counter_metric_t errors_total    {"myservice_errors_total",    "Total errors encountered"};
  gauge_metric_t   queue_depth     {"myservice_queue_depth",     "Current queue depth"};

public:

  MyService() = default;

  void handle_request() {
    requests_total++;
    queue_depth++;

    // Simulate some work...
    bool success = (std::rand() % 10) > 1;  // 90% success rate

    if (!success)
      errors_total++;

    queue_depth--;
  }
};

// =============================================================================
// main
// =============================================================================

int main() {
  std::cout << "=== Simple class instrumentation example ===\n";
  std::cout << "  Metrics available at http://localhost:9100/metrics\n\n";

  // One line — all metrics from all classes are exposed.
  http_server.start("127.0.0.1:9100");

  MyService service;

  for (int i = 0; i < 60; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    service.handle_request();

    if (i % 10 == 9)
      std::cout << "  tick " << (i + 1) << "/60\n" << global_registry.serialize() << std::endl;
  }

  return 0;
}