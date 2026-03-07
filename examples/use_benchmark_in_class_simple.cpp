/*
* prometheus-cpp-lite — header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* use_benchmark_in_class_simple.cpp — Using benchmark metrics inside a class
*
* Demonstrates how to measure wall-clock time of methods using benchmark_t.
* Each benchmark metric accumulates elapsed time across multiple start/stop
* cycles — perfect for profiling production code without external tools.
*
* Uses prometheus-cpp-lite-full target which provides pre-defined global objects.
*
* Best practice: each thread should have its own benchmark instance to avoid
* start/stop contention.  This example uses a single-threaded scenario for
* simplicity.
*/

#include <prometheus/prometheus.h>

using namespace prometheus;

// =============================================================================
// DataProcessor — a class with two phases, each profiled with a benchmark
// =============================================================================

class DataProcessor {

  // Shared family — all benchmarks grouped under one metric name.
  // Registered in global_registry automatically.
  family_t           processing_time {"data_processing_seconds", "Time spent in each processing phase"};
  benchmark_metric_t parse_timer     {processing_time, {{"phase", "parse"}}};
  benchmark_metric_t transform_timer {processing_time, {{"phase", "transform"}}};

public:

  DataProcessor() = default;

  void process() {
    // Phase 1: parse
    parse_timer.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100 + std::rand() % 200));
    parse_timer.stop();

    // Phase 2: transform
    transform_timer.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200 + std::rand() % 300));
    transform_timer.stop();
  }
};

// =============================================================================
// main
// =============================================================================

int main() {
  std::cout << "=== Benchmark class instrumentation example ===\n";
  std::cout << "  Metrics available at http://localhost:9100/metrics\n\n";

  http_server.start("127.0.0.1:9100");

  DataProcessor processor;

  for (int i = 0; i < 30; ++i) {
    processor.process();

    std::cout << "  iteration " << (i + 1) << "/30\n";
    if (i % 5 == 4)
      std::cout << global_registry.serialize() << std::endl;
  }

  return 0;
}
