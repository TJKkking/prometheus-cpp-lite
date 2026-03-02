
/*
*
* prometheus-cpp-lite - header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* test_summary.cpp - Summary metric usage examples
*
* This file demonstrates every supported way to create and use Summary metrics
* with the prometheus-cpp-lite library — from the shortest one-liner forms
* that rely on the global registry, through explicit typed/untyped family
* wrappers, to the legacy APIs compatible with prometheus-cpp and
* prometheus-cpp-lite-core.
*
* Test list
* ---------
* test_main_1   — Shortest path: global registry + implicit family (default quantiles).
* test_main_2   — Shortest path with custom quantile definitions.
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

#include "prometheus/summary.h"

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
// Helper: feed a fixed set of observations into a summary metric to produce
// meaningful quantile output.
// =============================================================================

static void feed_latency_observations(summary_metric_t& metric) {
  // Simulated latency samples (in seconds), roughly log-normal distributed.
  const double samples[] = {
    0.002, 0.003, 0.004, 0.005, 0.006, 0.007, 0.008, 0.010, 0.012, 0.015,
    0.018, 0.020, 0.022, 0.025, 0.030, 0.035, 0.040, 0.050, 0.060, 0.080,
    0.100, 0.120, 0.150, 0.200, 0.250, 0.300, 0.400, 0.500, 0.750, 1.000,
    1.200, 1.500, 2.000, 2.500, 3.000, 4.000, 5.000, 7.000, 8.000, 10.00
  };
  for (double s : samples)
    metric.Observe(s);
}

// =============================================================================
// test_main_1 - Shortest path with global registry and implicit family
//
// Chain: global_registry → (implicit family) → metric
//
// The simplest way to create a summary.  A family is created automatically
// behind the scenes inside the global registry.  Default quantiles are
// p50, p90, p95, p99.
// =============================================================================

void test_main_1() {
  std::cout << "\n=== test_main_1 - Shortest path with global registry and implicit family ===\n";

  summary_metric_t metric1 ("request_duration_seconds", "HTTP request duration");

  feed_latency_observations(metric1);

  std::cout << " - metric1 count: " << metric1.GetCount() << std::endl;
  std::cout << " - metric1 sum:   " << metric1.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_main_2 - Shortest path with custom quantile definitions
//
// Chain: global_registry → (implicit family) → metric
//
// Same as test_main_1, but demonstrates how to pass custom quantile/error
// pairs instead of the defaults.
// =============================================================================

void test_main_2() {
  std::cout << "\n=== test_main_2 - Shortest path with custom quantile definitions ===\n";

  summary_metric_t metric2 ("response_time_seconds", "API response time", {}, {{0.5, 0.05}, {0.75, 0.02}, {0.9, 0.01}, {0.99, 0.001}});

  feed_latency_observations(metric2);

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

  family_t         durations   ("handler_duration_seconds", "handler durations");
  summary_metric_t metric_get  (durations, {{"method",  "GET"}}, {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}});
  summary_metric_t metric_post (durations, {{"method", "POST"}}, {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}});

  // Feed observations to GET handler.
  for (double v : {0.01, 0.02, 0.05, 0.08, 0.1, 0.15, 0.3, 0.5, 1.0, 2.0})
    metric_get.Observe(v);

  // Feed observations to POST handler.
  for (double v : {0.02, 0.04, 0.07, 0.12, 0.2, 0.35, 0.6, 0.9, 1.5, 3.0})
    metric_post.Observe(v);

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

  summary_family_t durations     ("grpc_duration_seconds", "gRPC call durations");
  summary_metric_t metric_unary  (durations, {{"type",  "unary"}}, {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}});
  summary_metric_t metric_stream (durations, {{"type", "stream"}}, {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}});

  for (double v : {0.005, 0.01, 0.02, 0.03, 0.05, 0.08, 0.1, 0.2, 0.5, 1.0})
    metric_unary.Observe(v);

  for (double v : {0.1, 0.2, 0.5, 0.8, 1.0, 1.5, 2.0, 3.0, 5.0, 10.0})
    metric_stream.Observe(v);

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
// global one.
// =============================================================================

void test_main_5() {
  std::cout << "\n=== test_main_5 - User-owned registry with implicit family ===\n";

  registry_t       registry;
  summary_metric_t metric1 (registry, "task_duration_seconds", "task processing duration", {}, {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}});

  for (double v : {0.1, 0.2, 0.3, 0.5, 0.8, 1.0, 1.5, 2.0, 3.0, 5.0})
    metric1.Observe(v);

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

  registry_t       registry;
  family_t         durations    (registry, "db_query_duration_seconds", "database query durations", {{"host", "localhost"}});
  summary_metric_t metric_read  (durations, {{"operation",  "read"}}, {{0.5, 0.05}, {0.9, 0.01}, {0.95, 0.005}, {0.99, 0.001}});
  summary_metric_t metric_write (durations, {{"operation", "write"}}, {{0.5, 0.05}, {0.9, 0.01}, {0.95, 0.005}, {0.99, 0.001}});

  for (double v : {0.001, 0.002, 0.005, 0.008, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5})
    metric_read.Observe(v);

  for (double v : {0.005, 0.01, 0.02, 0.05, 0.08, 0.1, 0.2, 0.5, 1.0, 2.0})
    metric_write.Observe(v);

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

  std::shared_ptr<registry_t>    registry = std::make_shared<registry_t>();
  summary_family_t durations    (registry, "cache_latency_seconds", "cache operation latency", {{"host", "localhost"}});
  summary_metric_t metric_hit   (durations, {{"result",  "hit"}}, {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}});
  summary_metric_t metric_miss  (durations, {{"result", "miss"}}, {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}});

  for (double v : {0.0001, 0.0002, 0.0005, 0.001, 0.002, 0.005, 0.008, 0.01, 0.02, 0.05})
    metric_hit.Observe(v);

  for (double v : {0.01, 0.02, 0.05, 0.08, 0.1, 0.15, 0.2, 0.3, 0.5, 1.0})
    metric_miss.Observe(v);

  std::cout << " - metric_hit  count: " <<  metric_hit.GetCount() << std::endl;
  std::cout << " - metric_hit  sum:   " <<  metric_hit.GetSum()   << std::endl;
  std::cout << " - metric_miss count: " << metric_miss.GetCount() << std::endl;
  std::cout << " - metric_miss sum:   " << metric_miss.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << registry->serialize();
}



// =============================================================================
// test_legacy_1 - Legacy prometheus-cpp like API: standalone Builder + references
//
// Chain: Registry& → BuildSummary().Register() → CustomFamily& → Summary&
//
// Uses the fluent Builder to construct and register a CustomFamily, then
// obtains raw Summary references.
// =============================================================================

void test_legacy_1() {
  std::cout << "\n=== test_legacy_1 - Legacy prometheus-cpp like API: standalone Builder + references ===\n";

  typedef Summary<double>             DoubleSummary;
  typedef CustomFamily<DoubleSummary> MetricFamily;

  Registry      registry;
  MetricFamily& durations = BuildSummary().Name("request_duration_seconds")
                                          .Help("HTTP request duration")
                                          .Labels({{"host", "localhost"}, {"legacy", "yes"}})
                                          .Register(registry);

  SummaryQuantiles quantiles {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}};

  DoubleSummary& metric_api   = durations.Add({{"endpoint",   "api"}}, quantiles);
  DoubleSummary& metric_admin = durations.Add({{"endpoint", "admin"}}, quantiles);

  for (double v : {0.01, 0.02, 0.05, 0.1, 0.2, 0.3, 0.5, 0.8, 1.0, 2.0})
    metric_api.Observe(v);

  for (double v : {0.05, 0.1, 0.15, 0.2, 0.3, 0.5, 0.8, 1.0, 1.5, 3.0})
    metric_admin.Observe(v);

  std::cout << " - metric_api   count: " <<   metric_api.GetCount() << std::endl;
  std::cout << " - metric_api   sum:   " <<   metric_api.GetSum()   << std::endl;
  std::cout << " - metric_admin count: " << metric_admin.GetCount() << std::endl;
  std::cout << " - metric_admin sum:   " << metric_admin.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_2 - Legacy prometheus-cpp-lite-core API: CustomFamily::Build() static method
//
// Chain: Registry& → CustomFamily::Build() → CustomFamily& → Summary&
//
// Same as test_legacy_1 but uses the static Build() method on CustomFamily.
// =============================================================================

void test_legacy_2() {
  std::cout << "\n=== test_legacy_2 - Legacy prometheus-cpp-lite-core API: CustomFamily::Build() static method ===\n";

  typedef Summary<double>             DoubleSummary;
  typedef CustomFamily<DoubleSummary> MetricFamily;

  SummaryQuantiles quantiles {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}};

  Registry       registry;
  MetricFamily&  durations    = MetricFamily::Build(registry, "response_time_seconds", "response time", {{"host", "localhost"}});
  DoubleSummary& metric_fast  = durations.Add({{"tier", "fast"}}, quantiles);
  DoubleSummary& metric_slow  = durations.Add({{"tier", "slow"}}, quantiles);

  for (double v : {0.001, 0.002, 0.005, 0.008, 0.01, 0.02, 0.03, 0.05, 0.08, 0.1})
    metric_fast.Observe(v);

  for (double v : {0.1, 0.2, 0.5, 0.8, 1.0, 1.5, 2.0, 3.0, 5.0, 10.0})
    metric_slow.Observe(v);

  std::cout << " - metric_fast count: " << metric_fast.GetCount() << std::endl;
  std::cout << " - metric_fast sum:   " << metric_fast.GetSum()   << std::endl;
  std::cout << " - metric_slow count: " << metric_slow.GetCount() << std::endl;
  std::cout << " - metric_slow sum:   " << metric_slow.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_3 - Legacy prometheus-cpp-lite-core API: Registry::Add<MetricType>() typed registration
//
// Chain: Registry& → Registry::Add<Summary>() → CustomFamily& → Summary&
//
// Registers a typed family directly on the registry.
// =============================================================================

void test_legacy_3() {
  std::cout << "\n=== test_legacy_3 - Legacy prometheus-cpp-lite-core API: Registry::Add<MetricType>() typed registration ===\n";

  typedef Summary<double>             DoubleSummary;
  typedef CustomFamily<DoubleSummary> MetricFamily;

  SummaryQuantiles quantiles {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}};

  Registry       registry;
  MetricFamily&  durations      = registry.Add<DoubleSummary>("processing_duration_seconds", "processing time", {{"host", "localhost"}});
  DoubleSummary& metric_parse   = durations.Add({{"phase",   "parse"}}, quantiles);
  DoubleSummary& metric_execute = durations.Add({{"phase", "execute"}}, quantiles);

  for (double v : {0.005, 0.01, 0.015, 0.02, 0.03, 0.05, 0.08, 0.1, 0.15, 0.2})
    metric_parse.Observe(v);

  for (double v : {0.05, 0.1, 0.2, 0.3, 0.5, 0.8, 1.0, 1.5, 2.0, 5.0})
    metric_execute.Observe(v);

  std::cout << " - metric_parse   count: " <<   metric_parse.GetCount() << std::endl;
  std::cout << " - metric_parse   sum:   " <<   metric_parse.GetSum()   << std::endl;
  std::cout << " - metric_execute count: " << metric_execute.GetCount() << std::endl;
  std::cout << " - metric_execute sum:   " << metric_execute.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_4 - Legacy prometheus-cpp-lite-core API: untyped Family + explicit Family::Add<MetricType>()
//
// Chain: Registry& → Registry::Add() → Family& → Family::Add<Summary>()
//
// The most explicit form: the family is untyped, and every Add() call
// must specify the concrete metric type as a template argument.
// =============================================================================

void test_legacy_4() {
  std::cout << "\n=== test_legacy_4 - Legacy prometheus-cpp-lite-core API: untyped Family + explicit Family::Add<MetricType>() ===\n";

  SummaryQuantiles quantiles {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}};

  Registry          registry;
  Family&           durations    = registry.Add("io_duration_seconds", "I/O operation time", {{"host", "localhost"}});
  Summary<double>& metric_read  = durations.Add<Summary<double>>({{"operation",  "read"}}, quantiles);
  Summary<double>& metric_write = durations.Add<Summary<double>>({{"operation", "write"}}, quantiles);

  for (double v : {0.001, 0.003, 0.005, 0.01, 0.02, 0.05, 0.08, 0.1, 0.2, 0.5})
    metric_read.Observe(v);

  for (double v : {0.005, 0.01, 0.03, 0.05, 0.1, 0.15, 0.3, 0.5, 1.0, 2.0})
    metric_write.Observe(v);

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
  global_registry.RemoveAll();  // Clear global registry for a clean test.

  simpleapi::summary_metric_t metric1 { "request_latency", "request latency summary" };

  for (double v : {0.01, 0.02, 0.05, 0.1, 0.2, 0.3, 0.5, 0.8, 1.0, 2.0})
    metric1.Observe(v);

  std::cout << " - metric1 count: " << metric1.GetCount() << std::endl;
  std::cout << " - metric1 sum:   " << metric1.GetSum()   << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_legacy_6 - Legacy SimpleAPI: family wrapper + metric wrappers
//
// Chain: global_registry → simpleapi::summary_family_t → simpleapi::summary_metric_t
//
// Uses the simpleapi namespace aliases and family.Add() to create metrics.
// =============================================================================

void test_legacy_6() {
  std::cout << "\n=== test_legacy_6 - Legacy SimpleAPI: family wrapper + metric wrappers ===\n";

  global_registry.RemoveAll();  // Clear global registry for a clean test.

  SummaryQuantiles quantiles {{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}};

  simpleapi::summary_family_t metric_family { "simple_summary", "simple summary family example" };
  simpleapi::summary_metric_t metric1       { metric_family.Add({{"name", "summary1"}}, quantiles) };
  simpleapi::summary_metric_t metric2       { metric_family.Add({{"name", "summary2"}}, quantiles) };

  for (double v : {0.01, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 3.0, 5.0, 10.0})
    metric1.Observe(v);

  for (double v : {0.001, 0.005, 0.01, 0.02, 0.05, 0.08, 0.1, 0.15, 0.2, 0.5})
    metric2.Observe(v);

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
  std::cout << "=== Summary metric - all usage variants ===\n";

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