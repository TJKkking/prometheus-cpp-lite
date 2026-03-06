
/*
* prometheus-cpp-lite - header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* test_benchmark.cpp - Benchmark metric usage examples
*
* This file demonstrates every supported way to create and use Benchmark metrics
* with the prometheus-cpp-lite library — from the shortest one-liner forms
* that rely on the global registry, through explicit typed/untyped family
* wrappers, to the legacy APIs compatible with prometheus-cpp and
* prometheus-cpp-lite-core.
*
* Benchmark metric design philosophy
* -----------------------------------
* The benchmark_t metric is designed for per-thread elapsed time measurement.
* Each thread should own its own benchmark_metric_t instance (identified by a
* unique label, e.g. {"thread", "0"}), so that start()/stop() calls never
* race across threads.  The underlying atomic<double> accumulator allows
* safe serialization from any thread, but the start/stop state machine is
* intentionally NOT thread-safe — this is by design to avoid lock overhead
* in hot measurement paths.
*
* Test list
* ---------
* test_main_1   — Shortest path: global registry + implicit family.
* test_main_2   — Per-thread pattern: family with thread-id labels.
* test_main_3   — Explicit untyped family wrapper (global registry, runtime type check).
* test_main_4   — Explicit typed family wrapper (global registry, compile-time type safety).
* test_main_5   — User-owned registry with implicit family.
* test_main_6   — User-owned registry with explicit untyped family wrapper.
* test_main_7   — User-owned registry with typed family wrapper (compile-time type safety).
* test_legacy_1 — Legacy prometheus-cpp API: standalone Builder + references.
* test_legacy_2 — Legacy prometheus-cpp-lite-core API: CustomFamily::Build() static method.
* test_legacy_3 — Legacy prometheus-cpp-lite-core API: Registry::Add<MetricType>() typed registration.
* test_legacy_4 — Legacy prometheus-cpp-lite-core API: untyped Family + explicit Family::Add<MetricType>().
* test_legacy_5 — Legacy SimpleAPI: metric wrappers with global registry (shortest form).
* test_legacy_6 — Legacy SimpleAPI: family wrapper + metric wrappers.
*/

#include "prometheus/benchmark.h"

#include <thread>
#include <vector>
#include <functional>

using namespace prometheus;

// =============================================================================
// Global registry definition
//
// Required when using the shortened construction chains that rely on `global_registry`.
// In a real application this should be defined once in a single .cpp file.
// =============================================================================

namespace prometheus {
  registry_t global_registry;
}

// =============================================================================
// Helper: simulate work by sleeping for the given number of milliseconds.
// =============================================================================

static void simulate_work(int milliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// =============================================================================
// test_main_1 - Shortest path with global registry and implicit family
//
// Chain: global_registry → (implicit family) → metric
//
// The simplest way to create a benchmark.  A family is created automatically
// behind the scenes inside the global registry.  The default value type is double
// (elapsed seconds).
// =============================================================================

void test_main_1() {
  std::cout << "\n=== test_main_1 - Shortest path with global registry and implicit family ===\n";

  benchmark_metric_t metric1 ("task_duration_seconds", "elapsed time of task1");

  metric1.start();
  simulate_work(15);
  metric1.stop();

  metric1.start();
  simulate_work(10);
  metric1.stop();

  std::cout << " - metric1 accumulated time: " << metric1.Get() << " sec" << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_main_2 - Per-thread pattern: family with thread-id labels
//
// Chain: global_registry → family → metric (per thread)
//
// This is the RECOMMENDED usage pattern for benchmark_t.
//
// Each thread creates its own benchmark_metric_t with a unique label
// (e.g. thread index).  Since start()/stop() state is local to each
// instance, there is no contention between threads.  The accumulated
// elapsed time is stored in a per-metric atomic, so serialization
// from any thread is always safe.
//
// ┌──────────┐   ┌──────────────────────────┐
// │ Thread 0 │──▶│ metric {thread="0"}      │──▶ start/stop (no lock)
// └──────────┘   └──────────────────────────┘
// ┌──────────┐   ┌──────────────────────────┐
// │ Thread 1 │──▶│ metric {thread="1"}      │──▶ start/stop (no lock)
// └──────────┘   └──────────────────────────┘
// ┌──────────┐   ┌──────────────────────────┐
// │ Thread 2 │──▶│ metric {thread="2"}      │──▶ start/stop (no lock)
// └──────────┘   └──────────────────────────┘
//                         │
//                         ▼
//               ┌──────────────────┐
//               │ serialize() from │  (safe: reads atomic values)
//               │ any thread       │
//               └──────────────────┘
// =============================================================================

void test_main_2() {
  std::cout << "\n=== test_main_2 - Per-thread pattern: family with thread-id labels ===\n";

  benchmark_family_t worker_time ("worker_duration_seconds", "per-thread elapsed time");

  constexpr int NUM_THREADS = 4;
  std::vector<std::thread> threads;

  // Each thread gets its own metric identified by thread index.
  // No synchronization is needed for start()/stop() — each thread
  // exclusively owns its metric instance.
  for (int i = 0; i < NUM_THREADS; ++i) {
    threads.emplace_back([&worker_time, i]() {
      benchmark_metric_t my_metric(worker_time, {{"thread", std::to_string(i)}});

      // Simulate several work iterations.
      for (int iter = 0; iter < 3; ++iter) {
        my_metric.start();
        simulate_work(5 + i * 3);  // Different work per thread.
        my_metric.stop();
      }

      std::cout << " - thread " << i << " accumulated time: " << my_metric.Get() << " sec" << std::endl;
    });
  }

  for (auto& t : threads)
    t.join();

  // Serialization is safe from the main thread — reads atomic accumulators.
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_main_3 - Explicit untyped family wrapper (global registry)
//
// Chain: global_registry → family → metric
//
// Creates an untyped family_t wrapper.  The metric type check (ensuring all
// metrics in the family share the same concrete type) happens at runtime.
// =============================================================================

void test_main_3() {
  std::cout << "\n=== test_main_3 - Explicit untyped family wrapper (global registry) ===\n";

  family_t           durations   ("pipeline_duration_seconds", "pipeline stage durations");
  benchmark_metric_t metric_load (durations, {{"stage",    "load"}});
  benchmark_metric_t metric_proc (durations, {{"stage", "process"}});

  metric_load.start();
  simulate_work(10);
  metric_load.stop();

  metric_proc.start();
  simulate_work(20);
  metric_proc.stop();

  std::cout << " - metric_load time: " << metric_load.Get() << " sec" << std::endl;
  std::cout << " - metric_proc time: " << metric_proc.Get() << " sec" << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_main_4 - Explicit typed family wrapper (global registry, compile-time safety)
//
// Chain: global_registry → family → metric
//
// Uses typed family wrapper so the metric type is fixed at compile time —
// no runtime type mismatch is possible.
// =============================================================================

void test_main_4() {
  std::cout << "\n=== test_main_4 - Explicit typed family wrapper (global registry, compile-time safety) ===\n";

  benchmark_family_t durations     ("service_duration_seconds", "service call durations");
  benchmark_metric_t metric_auth   (durations, {{"service",  "auth"}});
  benchmark_metric_t metric_search (durations, {{"service", "search"}});

  metric_auth.start();
  simulate_work(8);
  metric_auth.stop();

  metric_search.start();
  simulate_work(25);
  metric_search.stop();

  std::cout << " - metric_auth   time: " <<   metric_auth.Get() << " sec" << std::endl;
  std::cout << " - metric_search time: " << metric_search.Get() << " sec" << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_main_5 - User-owned registry with implicit family
//
// Chain: registry → (implicit family) → metric
//
// Same as test_main_1, but with a locally created registry instead of the
// global one.
// =============================================================================

void test_main_5() {
  std::cout << "\n=== test_main_5 - User-owned registry with implicit family ===\n";

  registry_t         registry;
  benchmark_metric_t metric1 (registry, "job_duration_seconds", "job elapsed time");

  metric1.start();
  simulate_work(12);
  metric1.stop();

  std::cout << " - metric1 time: " << metric1.Get() << " sec" << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_main_6 - User-owned registry with explicit untyped family wrapper
//
// Chain: registry → family → metric
//
// Combines a user-owned registry with an explicit untyped family wrapper.
// =============================================================================

void test_main_6() {
  std::cout << "\n=== test_main_6 - User-owned registry with explicit untyped family ===\n";

  registry_t         registry;
  family_t           durations    (registry, "etl_duration_seconds", "ETL stage durations",
                                   {{"host", "localhost"}});
  benchmark_metric_t metric_extract  (durations, {{"stage", "extract"}});
  benchmark_metric_t metric_transform(durations, {{"stage", "transform"}});
  benchmark_metric_t metric_load     (durations, {{"stage", "load"}});

  metric_extract.start();
  simulate_work(10);
  metric_extract.stop();

  metric_transform.start();
  simulate_work(20);
  metric_transform.stop();

  metric_load.start();
  simulate_work(5);
  metric_load.stop();

  std::cout << " - metric_extract   time: " <<   metric_extract.Get() << " sec" << std::endl;
  std::cout << " - metric_transform time: " << metric_transform.Get() << " sec" << std::endl;
  std::cout << " - metric_load      time: " <<      metric_load.Get() << " sec" << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_main_7 - User-owned registry with typed family wrapper (compile-time safety)
//
// Chain: registry → family → metric
//
// Combines a user-owned registry with a typed family wrapper so the metric type
// is fixed at compile time — no runtime type mismatch is possible.
//
// Demonstrates the per-thread pattern with a user-owned registry.
// =============================================================================

void test_main_7() {
  std::cout << "\n=== test_main_7 - User-owned registry with typed family wrapper + per-thread pattern ===\n";

  std::shared_ptr<registry_t>   registry = std::make_shared<registry_t>();
  benchmark_family_t durations (registry, "render_duration_seconds", "per-thread render time", {{"host", "localhost"}});

  constexpr int NUM_THREADS = 3;
  std::vector<std::thread> threads;

  for (int i = 0; i < NUM_THREADS; ++i) {
    threads.emplace_back([&durations, i]() {
      benchmark_metric_t my_metric(durations, {{"thread", std::to_string(i)}});

      my_metric.start();
      simulate_work(10 + i * 5);
      my_metric.stop();

      std::cout << " - thread " << i << " render time: " << my_metric.Get() << " sec" << std::endl;
    });
  }

  for (auto& t : threads)
    t.join();

  std::cout << " - output serialized data:\n" << registry->serialize();
}



// =============================================================================
// test_legacy_1 - Legacy prometheus-cpp like API: standalone Builder + references
//
// Chain: Registry& → BuildBenchmark().Register() → CustomFamily& → Benchmark&
//
// Uses the fluent Builder to construct and register a CustomFamily, then
// obtains raw Benchmark references.
// =============================================================================

void test_legacy_1() {
  std::cout << "\n=== test_legacy_1 - Legacy prometheus-cpp like API: standalone Builder + references ===\n";

  typedef Benchmark<double>              DoubleBenchmark;
  typedef CustomFamily<DoubleBenchmark>  MetricFamily;

  Registry       registry;
  MetricFamily&  durations = BuildBenchmark().Name("compile_duration_seconds")
    .Help("compilation elapsed time")
    .Labels({{"host", "localhost"}, {"legacy", "yes"}})
    .Register(registry);

  DoubleBenchmark& metric_parse   = durations.Add({{"phase",   "parse"}});
  DoubleBenchmark& metric_codegen = durations.Add({{"phase", "codegen"}});

  metric_parse.start();
  simulate_work(15);
  metric_parse.stop();

  metric_codegen.start();
  simulate_work(30);
  metric_codegen.stop();

  std::cout << " - metric_parse   time: " <<   metric_parse.Get() << " sec" << std::endl;
  std::cout << " - metric_codegen time: " << metric_codegen.Get() << " sec" << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_2 - Legacy prometheus-cpp-lite-core API: CustomFamily::Build() static method
//
// Chain: Registry& → CustomFamily::Build() → CustomFamily& → Benchmark&
//
// Same as test_legacy_1 but uses the static Build() method on CustomFamily.
// =============================================================================

void test_legacy_2() {
  std::cout << "\n=== test_legacy_2 - Legacy prometheus-cpp-lite-core API: CustomFamily::Build() static method ===\n";

  typedef Benchmark<double>              DoubleBenchmark;
  typedef CustomFamily<DoubleBenchmark>  MetricFamily;

  Registry         registry;
  MetricFamily&    durations     = MetricFamily::Build(registry, "deploy_duration_seconds", "deployment time",
                                                       {{"host", "localhost"}});
  DoubleBenchmark& metric_build  = durations.Add({{"step",  "build"}});
  DoubleBenchmark& metric_deploy = durations.Add({{"step", "deploy"}});

  metric_build.start();
  simulate_work(20);
  metric_build.stop();

  metric_deploy.start();
  simulate_work(10);
  metric_deploy.stop();

  std::cout << " - metric_build  time: " <<  metric_build.Get() << " sec" << std::endl;
  std::cout << " - metric_deploy time: " << metric_deploy.Get() << " sec" << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_3 - Legacy prometheus-cpp-lite-core API: Registry::Add<MetricType>() typed registration
//
// Chain: Registry& → Registry::Add<Benchmark>() → CustomFamily& → Benchmark&
//
// Registers a typed family directly on the registry.
// =============================================================================

void test_legacy_3() {
  std::cout << "\n=== test_legacy_3 - Legacy prometheus-cpp-lite-core API: Registry::Add<MetricType>() typed registration ===\n";

  typedef Benchmark<double>              DoubleBenchmark;
  typedef CustomFamily<DoubleBenchmark>  MetricFamily;

  Registry         registry;
  MetricFamily&    durations    = registry.Add<DoubleBenchmark>("test_duration_seconds", "test suite timing",
                                                                {{"host", "localhost"}});
  DoubleBenchmark& metric_unit  = durations.Add({{"suite",        "unit"}});
  DoubleBenchmark& metric_integ = durations.Add({{"suite", "integration"}});

  metric_unit.start();
  simulate_work(8);
  metric_unit.stop();

  metric_integ.start();
  simulate_work(35);
  metric_integ.stop();

  std::cout << " - metric_unit  time: " <<  metric_unit.Get() << " sec" << std::endl;
  std::cout << " - metric_integ time: " << metric_integ.Get() << " sec" << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_4 - Legacy prometheus-cpp-lite-core API: untyped Family + explicit Family::Add<MetricType>()
//
// Chain: Registry& → Registry::Add() → Family& → Family::Add<Benchmark>()
//
// The most explicit form: the family is untyped, and every Add() call
// must specify the concrete metric type as a template argument.
// =============================================================================

void test_legacy_4() {
  std::cout << "\n=== test_legacy_4 - Legacy prometheus-cpp-lite-core API: untyped Family + explicit Family::Add<MetricType>() ===\n";

  Registry           registry;
  Family&            durations    = registry.Add("io_duration_seconds", "I/O elapsed time",
                                                 {{"host", "localhost"}});
  Benchmark<double>& metric_read  = durations.Add<Benchmark<double>>({{"operation",  "read"}});
  Benchmark<double>& metric_write = durations.Add<Benchmark<double>>({{"operation", "write"}});

  metric_read.start();
  simulate_work(12);
  metric_read.stop();

  metric_write.start();
  simulate_work(18);
  metric_write.stop();

  std::cout << " - metric_read  time: " <<  metric_read.Get() << " sec" << std::endl;
  std::cout << " - metric_write time: " << metric_write.Get() << " sec" << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}



// =============================================================================
// Legacy SimpleAPI examples
// =============================================================================

// suppress deprecation warnings - legacy API is intentionally used here to verify backward compatibility
#ifdef _MSC_VER
#pragma warning(disable: 4996)  // deprecated declaration
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <prometheus/simpleapi.h>

// =============================================================================
// test_legacy_5 - Legacy SimpleAPI: metric wrappers with global registry (shortest form)
//
// Chain: global_registry → (implicit family) → metric
//
// Identical to test_main_1 but uses simpleapi namespace aliases.
// =============================================================================

void test_legacy_5() {
  std::cout << "\n=== test_legacy_5 - Legacy SimpleAPI: metric wrappers with global registry (shortest form) ===\n";
  global_registry.RemoveAll();  // Clear global registry for a clean test.

  simpleapi::benchmark_metric_t metric1 { "operation_duration", "operation elapsed time" };

  metric1.start();
  simulate_work(15);
  metric1.stop();

  std::cout << " - metric1 time: " << metric1.Get() << " sec" << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_legacy_6 - Legacy SimpleAPI: family wrapper + metric wrappers
//
// Chain: global_registry → simpleapi::benchmark_family_t → simpleapi::benchmark_metric_t
//
// Uses the simpleapi namespace aliases and family.Add() to create metrics.
// Demonstrates per-thread ownership: each worker gets its own metric handle.
// =============================================================================

void test_legacy_6() {
  std::cout << "\n=== test_legacy_6 - Legacy SimpleAPI: family wrapper + metric wrappers ===\n";

  global_registry.RemoveAll();  // Clear global registry for a clean test.

  simpleapi::benchmark_family_t metric_family { "worker_elapsed", "per-worker elapsed time" };

  constexpr int NUM_THREADS = 2;
  std::vector<std::thread> threads;

  for (int i = 0; i < NUM_THREADS; ++i) {
    threads.emplace_back([&metric_family, i]() {
      // Each thread owns its metric — no start/stop contention.
      simpleapi::benchmark_metric_t my_metric { metric_family.Add({{"worker", std::to_string(i)}}) };

      my_metric.start();
      simulate_work(10 + i * 10);
      my_metric.stop();

      std::cout << " - worker " << i << " time: " << my_metric.Get() << " sec" << std::endl;
    });
  }

  for (auto& t : threads)
    t.join();

  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// main
// =============================================================================

int main() {
  std::cout << "=== Benchmark metric - all usage variants ===\n";

  try {
    test_main_1();
    test_main_2();
    test_main_3();
    test_main_4();
    test_main_5();
    test_main_6();
    test_main_7();
    test_legacy_1();
    test_legacy_2();
    test_legacy_3();
    test_legacy_4();
    test_legacy_5();
    test_legacy_6();
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}