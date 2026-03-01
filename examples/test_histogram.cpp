
/*
* prometheus-cpp-lite - header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* test_histogram.cpp - Histogram metric usage examples
*
* This file demonstrates every supported way to create and use Histogram metrics
* with the prometheus-cpp-lite library — from the shortest one-liner forms
* that rely on the global registry, through explicit typed/untyped family
* wrappers, to the legacy APIs compatible with prometheus-cpp and
* prometheus-cpp-lite-core.
*
* Test list
* ---------
* test_main_1   — Shortest path: global registry + implicit family (double, default boundaries).
* test_main_2   — Shortest path with custom bucket boundaries.
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

#include "prometheus/histogram.h"

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
// test_main_1 - Shortest path with global registry and implicit family
//
// Chain: global_registry → (implicit family) → metric
//
// The simplest way to create a histogram.  A family is created automatically
// behind the scenes inside the global registry.  The default value type for
// histogram metric is double, with the Prometheus-recommended default bucket
// boundaries.
// =============================================================================

void test_main_1() {
  std::cout << "\n=== test_main_1 - Shortest path with global registry and implicit family ===\n";

  histogram_metric_t metric1 ("request_duration_seconds", "duration of HTTP requests");

  metric1.Observe(0.003);
  metric1.Observe(0.05);
  metric1.Observe(0.27);
  metric1.Observe(1.5);
  metric1.Observe(12.0);

  std::cout << " - metric1 count: " << metric1.GetCount() << std::endl;
  std::cout << " - metric1 sum:   " << metric1.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_main_2 - Shortest path with custom bucket boundaries
//
// Chain: global_registry → (implicit family) → metric
//
// Same as test_main_1, but demonstrates how to pass custom bucket boundaries
// instead of the Prometheus defaults.
// =============================================================================

void test_main_2() {
  std::cout << "\n=== test_main_2 - Shortest path with custom bucket boundaries ===\n";

  histogram_metric_t metric2 ("response_size_bytes", "size of HTTP responses", {}, {100, 500, 1000, 5000, 10000});

  metric2.Observe(50);
  metric2.Observe(250);
  metric2.Observe(750);
  metric2.Observe(3000);
  metric2.Observe(15000);

  std::cout << " - metric2 count: " << metric2.GetCount() << std::endl;
  std::cout << " - metric2 sum:   " << metric2.GetSum()   << std::endl;
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

  family_t           durations  ("http_duration_seconds", "HTTP request durations");
  histogram_metric_t metric_get (durations, {{"method",  "GET"}}, {0.01, 0.05, 0.1, 0.5, 1.0});
  histogram_metric_t metric_post(durations, {{"method", "POST"}}, {0.01, 0.05, 0.1, 0.5, 1.0});

  metric_get.Observe(0.03);
  metric_get.Observe(0.12);

  metric_post.Observe(0.08);
  metric_post.Observe(0.75);

  std::cout << " - metric_get  count: " <<  metric_get.GetCount() << std::endl;
  std::cout << " - metric_get  sum:   " <<  metric_get.GetSum()   << std::endl;
  std::cout << " - metric_post count: " << metric_post.GetCount() << std::endl;
  std::cout << " - metric_post sum:   " << metric_post.GetSum()   << std::endl;
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

  histogram_family_t durations    ("grpc_duration_seconds", "gRPC request durations");
  histogram_metric_t metric_unary (durations, {{"type",  "unary"}}, {0.01, 0.05, 0.1, 0.5, 1.0});
  histogram_metric_t metric_stream(durations, {{"type", "stream"}}, {0.01, 0.05, 0.1, 0.5, 1.0});

  metric_unary.Observe(0.02);
  metric_unary.Observe(0.15);

  metric_stream.Observe(0.5);
  metric_stream.Observe(2.0);

  std::cout << " - metric_unary  count: " <<  metric_unary.GetCount() << std::endl;
  std::cout << " - metric_unary  sum:   " <<  metric_unary.GetSum()   << std::endl;
  std::cout << " - metric_stream count: " << metric_stream.GetCount() << std::endl;
  std::cout << " - metric_stream sum:   " << metric_stream.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_main_5 - User-owned registry with implicit family
//
// Chain: registry → (implicit family) → metric
//
// Same as test_main_1, but with a locally created registry instead of the
// global one.  Useful when you need isolated metric namespaces (e.g. per-module).
// =============================================================================

void test_main_5() {
  std::cout << "\n=== test_main_5 - User-owned registry with implicit family ===\n";

  registry_t         registry;
  histogram_metric_t metric1 (registry, "task_duration_seconds", "task processing duration",
                              {}, {0.1, 0.5, 1.0, 5.0, 10.0});

  metric1.Observe(0.3);
  metric1.Observe(1.2);
  metric1.Observe(7.5);

  std::cout << " - metric1 count: " << metric1.GetCount() << std::endl;
  std::cout << " - metric1 sum:   " << metric1.GetSum()   << std::endl;
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
  family_t           durations   (registry, "db_query_duration_seconds", "database query durations",
                                  {{"host", "localhost"}});
  histogram_metric_t metric_read (durations, {{"operation",  "read"}}, {0.001, 0.01, 0.1, 1.0});
  histogram_metric_t metric_write(durations, {{"operation", "write"}}, {0.001, 0.01, 0.1, 1.0});

  metric_read.Observe(0.005);
  metric_read.Observe(0.03);
  metric_read.Observe(0.25);

  metric_write.Observe(0.008);
  metric_write.Observe(0.15);

  std::cout << " - metric_read  count: " <<  metric_read.GetCount() << std::endl;
  std::cout << " - metric_read  sum:   " <<  metric_read.GetSum()   << std::endl;
  std::cout << " - metric_write count: " << metric_write.GetCount() << std::endl;
  std::cout << " - metric_write sum:   " << metric_write.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_main_7 - User-owned registry with typed family wrapper (compile-time safety)
//
// Chain: registry → family → metric
//
// Combines a user-owned registry with a typed family wrapper so the metric type
// is fixed at compile time — no runtime type mismatch is possible.
// =============================================================================

void test_main_7() {
  std::cout << "\n=== test_main_7 - User-owned registry with typed family wrapper (compile-time safety) ===\n";

  std::shared_ptr<registry_t>      registry = std::make_shared<registry_t>();
  histogram_family_t durations    (registry, "cache_latency_seconds", "cache operation latency",
                                   {{"host", "localhost"}});
  histogram_metric_t metric_hit   (durations, {{"result",  "hit"}}, {0.0001, 0.001, 0.01, 0.1});
  histogram_metric_t metric_miss  (durations, {{"result", "miss"}}, {0.0001, 0.001, 0.01, 0.1});

  metric_hit.Observe(0.00005);
  metric_hit.Observe(0.0008);

  metric_miss.Observe(0.005);
  metric_miss.Observe(0.05);

  std::cout << " - metric_hit  count: " <<  metric_hit.GetCount() << std::endl;
  std::cout << " - metric_hit  sum:   " <<  metric_hit.GetSum()   << std::endl;
  std::cout << " - metric_miss count: " << metric_miss.GetCount() << std::endl;
  std::cout << " - metric_miss sum:   " << metric_miss.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << registry->serialize();
}



// =============================================================================
// test_legacy_1 - Legacy prometheus-cpp like API: standalone Builder + references
//
// Chain: Registry& → BuildHistogram().Register() → CustomFamily& → Histogram&
//
// Uses the fluent Builder to construct and register a CustomFamily, then
// obtains raw Histogram references.  This mirrors the prometheus-cpp style.
// =============================================================================

void test_legacy_1() {
  std::cout << "\n=== test_legacy_1 - Legacy prometheus-cpp like API: standalone Builder + references ===\n";

  typedef Histogram<double>             DoubleHistogram;
  typedef CustomFamily<DoubleHistogram> MetricFamily;

  Registry      registry;
  MetricFamily& durations = BuildHistogram().Name("request_duration_seconds")
    .Help("duration of HTTP requests")
    .Labels({{"host", "localhost"}, {"legacy", "yes"}})
    .Register(registry);

  DoubleHistogram& metric_api   = durations.Add({{"endpoint", "api"}},   BucketBoundaries{0.01, 0.05, 0.1, 0.5, 1.0});
  DoubleHistogram& metric_admin = durations.Add({{"endpoint", "admin"}}, BucketBoundaries{0.01, 0.05, 0.1, 0.5, 1.0});

  metric_api.Observe(0.03);
  metric_api.Observe(0.12);
  metric_api.Observe(0.75);

  metric_admin.Observe(0.08);
  metric_admin.Observe(1.5);

  std::cout << " - metric_api   count: " <<   metric_api.GetCount() << std::endl;
  std::cout << " - metric_api   sum:   " <<   metric_api.GetSum()   << std::endl;
  std::cout << " - metric_admin count: " << metric_admin.GetCount() << std::endl;
  std::cout << " - metric_admin sum:   " << metric_admin.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_2 - Legacy prometheus-cpp-lite-core API: CustomFamily::Build() static method
//
// Chain: Registry& → CustomFamily::Build() → CustomFamily& → Histogram&
//
// Same as test_legacy_1 but uses the static Build() method on CustomFamily instead
// of the standalone Builder.
// =============================================================================

void test_legacy_2() {
  std::cout << "\n=== test_legacy_2 - Legacy prometheus-cpp-lite-core API: CustomFamily::Build() static method ===\n";

  typedef Histogram<double>             DoubleHistogram;
  typedef CustomFamily<DoubleHistogram> MetricFamily;

  Registry         registry;
  MetricFamily&    durations    = MetricFamily::Build(registry, "request_duration_seconds", "description",
                                                      {{"host", "localhost"}});
  DoubleHistogram& metric_get   = durations.Add({{"method",  "GET"}}, BucketBoundaries{0.01, 0.1, 0.5, 1.0, 5.0});
  DoubleHistogram& metric_post  = durations.Add({{"method", "POST"}}, BucketBoundaries{0.01, 0.1, 0.5, 1.0, 5.0});

  metric_get.Observe(0.05);
  metric_get.Observe(0.3);

  metric_post.Observe(0.2);
  metric_post.Observe(2.5);

  std::cout << " - metric_get  count: " <<  metric_get.GetCount() << std::endl;
  std::cout << " - metric_get  sum:   " <<  metric_get.GetSum()   << std::endl;
  std::cout << " - metric_post count: " << metric_post.GetCount() << std::endl;
  std::cout << " - metric_post sum:   " << metric_post.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_3 - Legacy prometheus-cpp-lite-core API: Registry::Add<MetricType>() typed registration
//
// Chain: Registry& → Registry::Add<Histogram>() → CustomFamily& → Histogram&
//
// Registers a typed family directly on the registry.  The family remembers
// the metric type so Add() does not need an explicit template argument.
// =============================================================================

void test_legacy_3() {
  std::cout << "\n=== test_legacy_3 - Legacy prometheus-cpp-lite-core API: Registry::Add<MetricType>() typed registration ===\n";

  typedef Histogram<double>             DoubleHistogram;
  typedef CustomFamily<DoubleHistogram> MetricFamily;

  Registry         registry;
  MetricFamily&    durations   = registry.Add<DoubleHistogram>("response_time_seconds", "description",
                                                               {{"host", "localhost"}});
  DoubleHistogram& metric_fast = durations.Add({{"tier", "fast"}}, BucketBoundaries{0.001, 0.005, 0.01, 0.05, 0.1});
  DoubleHistogram& metric_slow = durations.Add({{"tier", "slow"}}, BucketBoundaries{0.1, 0.5, 1.0, 5.0, 10.0});

  metric_fast.Observe(0.002);
  metric_fast.Observe(0.008);
  metric_fast.Observe(0.03);

  metric_slow.Observe(0.3);
  metric_slow.Observe(2.5);
  metric_slow.Observe(7.0);

  std::cout << " - metric_fast count: " << metric_fast.GetCount() << std::endl;
  std::cout << " - metric_fast sum:   " << metric_fast.GetSum()   << std::endl;
  std::cout << " - metric_slow count: " << metric_slow.GetCount() << std::endl;
  std::cout << " - metric_slow sum:   " << metric_slow.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_4 - Legacy prometheus-cpp-lite-core API: untyped Family + explicit Family::Add<MetricType>()
//
// Chain: Registry& → Registry::Add() → Family& → Family::Add<Histogram>()
//
// The most explicit form: the family is untyped, and every Add() call
// must specify the concrete metric type as a template argument.
// =============================================================================

void test_legacy_4() {
  std::cout << "\n=== test_legacy_4 - Legacy prometheus-cpp-lite-core API: untyped Family + explicit Family::Add<MetricType>() ===\n";

  Registry          registry;
  Family&           durations     = registry.Add("io_latency_seconds", "description", {{"host", "localhost"}});
  Histogram<double>& metric_read  = durations.Add<Histogram<double>>({{"operation",  "read"}}, BucketBoundaries{0.001, 0.01, 0.1, 1.0});
  Histogram<double>& metric_write = durations.Add<Histogram<double>>({{"operation", "write"}}, BucketBoundaries{0.001, 0.01, 0.1, 1.0});

  metric_read.Observe(0.005);
  metric_read.Observe(0.08);

  metric_write.Observe(0.015);
  metric_write.Observe(0.5);

  std::cout << " - metric_read  count: " <<  metric_read.GetCount() << std::endl;
  std::cout << " - metric_read  sum:   " <<  metric_read.GetSum()   << std::endl;
  std::cout << " - metric_write count: " << metric_write.GetCount() << std::endl;
  std::cout << " - metric_write sum:   " << metric_write.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}



// =============================================================================
// Legacy SimpleAPI examples
// =============================================================================

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
  global_registry = Registry();  // Clear global registry for a clean test.

  simpleapi::histogram_metric_t metric1 { "request_duration", "request duration histogram" };

  metric1.Observe(0.01);
  metric1.Observe(0.05);
  metric1.Observe(0.3);
  metric1.Observe(1.5);

  std::cout << " - metric1 count: " << metric1.GetCount() << std::endl;
  std::cout << " - metric1 sum:   " << metric1.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_legacy_6 - Legacy SimpleAPI: family wrapper + metric wrappers
//
// Chain: global_registry → simpleapi::histogram_family_t → simpleapi::histogram_metric_t
//
// Uses the simpleapi namespace aliases and family.Add() to create metrics.
// =============================================================================

void test_legacy_6() {
  std::cout << "\n=== test_legacy_6 - Legacy SimpleAPI: family wrapper + metric wrappers ===\n";

  global_registry = Registry();  // Clear global registry for a clean test.

  simpleapi::histogram_family_t metric_family { "simple_histogram", "simple histogram family example" };
  simpleapi::histogram_metric_t metric1       { metric_family.Add({{"name", "hist1"}}, BucketBoundaries{0.1, 0.5, 1.0, 5.0}) };
  simpleapi::histogram_metric_t metric2       { metric_family.Add({{"name", "hist2"}}, BucketBoundaries{0.1, 0.5, 1.0, 5.0}) };

  metric1.Observe(0.3);
  metric1.Observe(0.8);

  metric2.Observe(2.0);
  metric2.Observe(7.0);

  std::cout << " - metric1 count: " << metric1.GetCount() << std::endl;
  std::cout << " - metric1 sum:   " << metric1.GetSum()   << std::endl;
  std::cout << " - metric2 count: " << metric2.GetCount() << std::endl;
  std::cout << " - metric2 sum:   " << metric2.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// main
// =============================================================================

int main() {
  std::cout << "=== Histogram metric - all usage variants ===\n";

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