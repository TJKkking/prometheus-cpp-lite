/*
* prometheus-cpp-lite - header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* test_gauge.cpp - Gauge metric usage examples
*
* This file demonstrates every supported way to create and use Gauge metrics
* with the prometheus-cpp-lite library — from the shortest one-liner forms
* that rely on the global registry, through explicit typed/untyped family
* wrappers, to the legacy APIs compatible with prometheus-cpp and
* prometheus-cpp-lite-core.
*
* Test list
* ---------
* test_main_1   — Shortest path: global registry + implicit family (int64_t).
* test_main_2   — Shortest path with custom value type (double).
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
* test_legacy_7 — Legacy prometheus-cpp-lite API: full explicit chain (integer + floating-point families).
*/

#include "prometheus/gauge.h"

using namespace prometheus;

// =============================================================================
// Global registry definition
//
// Required when using the shortened construction chains that rely on `global_registry` (test1–test4, test11–test12).
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
// The simplest way to create a gauge.  A family is created automatically
// behind the scenes inside the global registry. The default value type for
// gauge metric is int64_t.
// =============================================================================

void test_main_1() {
  std::cout << "\n=== test_main_1 - Shortest path with global registry and implicit family ===\n";

  gauge_metric_t metric1 ("metric1_name", "description of metric1");
  gauge_metric_t metric2 ("metric2_name", "description of metric2");

  metric1++;
  metric2 += 10;
  metric2--;
  metric2 -= 3;

  std::cout << " - metric1 value: " << metric1.Get() << std::endl;
  std::cout << " - metric2 value: " << metric2.Get() << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_main_2 - Shortest path with a custom value type (double)
//
// Chain: global_registry → (implicit family) → metric
//
// Same as test_main_1, but demonstrate how to use gauge_t<double&> to store
// a floating-point value instead of the default int64_t.
// =============================================================================

void test_main_2() {
  std::cout << "\n=== test_main_2 - Shortest path with a custom value type (double) ===\n";

  using double_metric_t = gauge_t<double&>;

  double_metric_t metric3 ("metric3_name", "description of metric3");
  double_metric_t metric4 ("metric4_name", "description of metric4");

  metric3 = 0.5;
  metric3 += 2.5;
  metric3.Decrement(0.3);

  metric4 += 10.0;
  metric4 -= 3.5;

  std::cout << " - metric3 value: " << metric3.Get() << std::endl;
  std::cout << " - metric4 value: " << metric4.Get() << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_main_3 - Explicit untyped family wrapper (global registry)
//
// Chain: global_registry → family → metric
//
// Creates an untyped family_t wrapper. The metric type check (ensuring all
// metrics in the family share the same concrete type) happens at runtime.
// =============================================================================

void test_main_3() {
  std::cout << "\n=== test_main_3 - Explicit untyped family wrapper (global registry) ===\n";

  family_t       temperature ("temperature", "some description of this family");
  gauge_metric_t metric_cpu  (temperature, {{"sensor", "cpu"}});
  gauge_metric_t metric_gpu  (temperature, {{"sensor", "gpu"}});

  // If you uncomment this, you will get a runtime exception because the check will happen at runtime:
  // ( double type of value not allowed in a family of gauge_t<int64_t> )
  //gauge_t<double&> metric_ambient (temperature, {{"sensor", "ambient"}});

  metric_cpu += 45;
  metric_gpu += 72;
  metric_gpu -= 5;

  std::cout << " - metric_cpu value: " << metric_cpu.Get() << std::endl;
  std::cout << " - metric_gpu value: " << metric_gpu.Get() << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_main_4 - Explicit typed family wrapper (global registry, compile-time safety)
//
// Chain: global_registry → family → metric
//
// Uses typed family wrapper so the metric type is fixed at compile time - no runtime type mismatch is possible.
// =============================================================================

void test_main_4() {
  std::cout << "\n=== test_main_4 - Explicit typed family wrapper (global registry, compile-time safety) ===\n";

  gauge_family_t active_connections ("active_connections", "description");
  gauge_metric_t metric_http        (active_connections, {{"protocol", "http"}});
  gauge_metric_t metric_grpc        (active_connections, {{"protocol", "grpc"}});

  // If you uncomment this, you'll get a type mismatch error right in the IDE, because the check will happen at compile time:
  // ( int64_t type of value not allowed in a family of gauge_t<double> )
  //gauge_t<double&> metric_ws (active_connections, {{"protocol", "ws"}});

  metric_http += 100;
  metric_http -= 20;
  metric_grpc += 50;
  metric_grpc.Decrement(10);

  std::cout << " - metric_http value: " << metric_http.Get() << std::endl;
  std::cout << " - metric_grpc value: " << metric_grpc.Get() << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_main_5 - User-owned registry with implicit family
//
// Chain: registry → (implicit family) → metric
//
// Same as test_main_1, but with a locally created registry instead of the global one.
// Useful when you need isolated metric namespaces (e.g. per-module).
// =============================================================================

void test_main_5() {
  std::cout << "\n=== test_main_5 - User-owned registry with implicit family ===\n";

  registry_t     registry;
  gauge_metric_t metric1 (registry, "metric1_name", "description1");
  gauge_metric_t metric2 (registry, "metric2_name", "description2");

  metric1++;
  metric1++;
  metric1--;

  metric2 += 10;
  metric2.Decrement(3);

  std::cout << " - metric1 value: " << metric1.Get() << std::endl;
  std::cout << " - metric2 value: " << metric2.Get() << std::endl;
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

  registry_t     registry;
  family_t       queue_size  (registry, "queue_size", "description", {{"host", "localhost"}});
  gauge_metric_t metric_in   (queue_size, {{"queue", "inbound"}});
  gauge_metric_t metric_out  (queue_size, {{"queue", "outbound"}});

  // If you uncomment this, you will get a runtime inconsistency exception because the check will happen at runtime:
  // ( double type of value not allowed in a family of gauge_t<int64_t> )
  //gauge_t<double&> metric_priority (queue_size, {{"queue", "priority"}});

  metric_in += 50;
  metric_in -= 12;
  metric_out += 30;
  metric_out.Decrement(5);

  std::cout << " - metric_in  value: " <<  metric_in.Get() << std::endl;
  std::cout << " - metric_out value: " << metric_out.Get() << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_main_7 - User-owned registry with typed family wrapper (compile-time safety)
//
// Chain: registry → family → metric
//
// Combines a user-owned registry with a typed family wrapper so the metric type
// is fixed at compile time - no runtime type mismatch is possible.
// =============================================================================

void test_main_7() {
  std::cout << "\n=== test_main_7 - User-owned registry with typed family wrapper (compile-time safety) ===\n";
  using gauge_double_t  = gauge_t<double&>;
  using family_double_t = custom_family_t<gauge_t<double>>;

  std::shared_ptr<registry_t>     registry = std::make_shared<registry_t>();
  family_double_t cpu_usage      (registry, "cpu_usage_percent", "description", {{"host", "localhost"}});
  gauge_double_t  metric_core0   (cpu_usage, {{"core", "0"}});
  gauge_double_t  metric_core1   (cpu_usage, {{"core", "1"}});

  // If you uncomment this, you'll get a type mismatch error right in the IDE, because the check will happen at compile time:
  // ( int64_t type of value not allowed in a family of gauge_t<double> )
  //gauge_metric_t metric_core2 (cpu_usage, {{"core", "2"}});

  metric_core0.Set(75.5);
  metric_core0 -= 10.2;

  metric_core1.Set(42.3);
  metric_core1.Decrement(2.1);

  std::cout << " - metric_core0 value: " << metric_core0.Get() << std::endl;
  std::cout << " - metric_core1 value: " << metric_core1.Get() << std::endl;
  std::cout << " - output serialized data:\n" << registry->serialize();
}



// =============================================================================
// test_legacy_1 - Legacy prometheus-cpp like API: standalone Builder + references
//
// Chain: Registry& → BuildGauge().Register() → CustomFamily& → Gauge&
//
// Uses the fluent Builder to construct and register a CustomFamily, then
// obtains raw Gauge references.  This mirrors the prometheus-cpp style.
// =============================================================================

void test_legacy_1() {
  std::cout << "\n=== test_legacy_1 - Legacy prometheus-cpp like API: standalone Builder + references ===\n";

  typedef Gauge<double>             DoubleGauge;
  typedef CustomFamily<DoubleGauge> MetricFamily;

  Registry      registry;
  MetricFamily& cpu_usage = BuildGauge().Name("cpu_usage_percent")
                                        .Help("description")
                                        .Labels({{"host", "localhost"}, {"legacy", "yes"}})
                                        .Register(registry);

  DoubleGauge& metric_core0 = cpu_usage.Add({{"core", "0"}});
  DoubleGauge& metric_core1 = cpu_usage.Add({{"core", "1"}});

  metric_core0.Set(85.5);
  metric_core0.Decrement(10.0);

  metric_core1.Set(50.0);
  metric_core1 -= 5.5;

  std::cout << " - metric_core0 value: " << metric_core0.Get() << std::endl;
  std::cout << " - metric_core1 value: " << metric_core1.Get() << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_2 - Legacy prometheus-cpp-lite-core API: CustomFamily::Build() static method
//
// Chain: Registry& → CustomFamily::Build() → CustomFamily& → Gauge&
//
// Same as test_legacy_1 but uses the static Build() method on CustomFamily instead
// of the standalone Builder.
// =============================================================================

void test_legacy_2() {
  std::cout << "\n=== test_legacy_2 - Legacy prometheus-cpp-lite-core API: CustomFamily::Build() static method ===\n";

  typedef Gauge<double>             DoubleGauge;
  typedef CustomFamily<DoubleGauge> MetricFamily;

  Registry      registry;
  MetricFamily& cpu_usage      = MetricFamily::Build(registry, "cpu_usage_percent", "description", {{"host", "localhost"}});
  DoubleGauge&  metric_core0   = cpu_usage.Add({{"core", "0"}});
  DoubleGauge&  metric_core1   = cpu_usage.Add({{"core", "1"}});

  metric_core0.Set(90.0);
  metric_core0 -= 15.5;

  metric_core1.Increment(60.0);
  metric_core1.Decrement(8.3);

  std::cout << " - metric_core0 value: " << metric_core0.Get() << std::endl;
  std::cout << " - metric_core1 value: " << metric_core1.Get() << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_3 - Legacy prometheus-cpp-lite-core API: Registry::Add<MetricType>() typed registration
//
// Chain: Registry& → Registry::Add<Gauge>() → CustomFamily& → Gauge&
//
// Registers a typed family directly on the registry.  The family remembers
// the metric type so Add() does not need an explicit template argument.
// =============================================================================

void test_legacy_3() {
  std::cout << "\n=== test_legacy_3 - Legacy prometheus-cpp-lite-core API: Registry::Add<MetricType>() typed registration ===\n";

  typedef Gauge<int64_t>            IntegerGauge;
  typedef CustomFamily<IntegerGauge> MetricFamily;

  Registry      registry;
  MetricFamily& active_sessions = registry.Add<IntegerGauge>("active_sessions", "description", {{"host", "localhost"}});
  IntegerGauge& metric_web      = active_sessions.Add({{"service", "web"}});
  IntegerGauge& metric_api      = active_sessions.Add({{"service", "api"}});

  metric_web.Set(100);
  metric_web.Decrement(15);

  metric_api.Increment(50);
  metric_api--;

  std::cout << " - metric_web value: " << metric_web.Get() << std::endl;
  std::cout << " - metric_api value: " << metric_api.Get() << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_4 - Legacy prometheus-cpp-lite-core API: untyped Family + explicit Family::Add<MetricType>()
//
// Chain: Registry& → Registry::Add() → Family& → Family::Add<Gauge>()
//
// The most explicit form: the family is untyped, and every Add() call
// must specify the concrete metric type as a template argument.
// =============================================================================

void test_legacy_4() {
  std::cout << "\n=== test_legacy_4 - Legacy prometheus-cpp-lite-core API: untyped Family + explicit Family::Add<MetricType>() ===\n";

  Registry        registry;
  Family&         memory     = registry.Add("memory_usage_bytes", "description", {{"host", "localhost"}});
  Gauge<double>&  metric_heap  = memory.Add<Gauge<double>>({{"region", "heap"}});
  Gauge<double>&  metric_stack = memory.Add<Gauge<double>>({{"region", "stack"}});

  metric_heap.Set(1024.5);
  metric_heap -= 100.0;

  metric_stack.Set(256.0);
  metric_stack.Decrement(32.5);

  std::cout << " - metric_heap  value: " << metric_heap.Get() << std::endl;
  std::cout << " - metric_stack value: " << metric_stack.Get() << std::endl;
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

  simpleapi::gauge_metric_t metric1 { "metric1", "metric_description" };
  simpleapi::gauge_metric_t metric2 { "metric2", "metric_description" };

  metric1++;
  metric1--;

  metric2 += 10;
  metric2 -= 3;

  std::cout << " - metric1 value: " << metric1.Get() << std::endl;
  std::cout << " - metric2 value: " << metric2.Get() << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_legacy_6 - Legacy SimpleAPI: family wrapper + metric wrappers
//
// Chain: global_registry → simpleapi::gauge_family_t → simpleapi::gauge_metric_t
//
// Uses the simpleapi namespace aliases and family.Add() to create metrics.
// =============================================================================

void test_legacy_6() {
  std::cout << "\n=== test_legacy_6 - Legacy SimpleAPI: family wrapper + metric wrappers ===\n";

  global_registry = Registry();  // Clear global registry for a clean test.

  simpleapi::gauge_family_t metric_family { "simple_family", "simple family example"  };
  simpleapi::gauge_metric_t metric1       { metric_family.Add({{"name", "gauge1"}}) };
  simpleapi::gauge_metric_t metric2       { metric_family.Add({{"name", "gauge2"}}) };

  metric1 += 5;
  metric1.Decrement(2);

  metric2++;
  metric2--;

  std::cout << " - metric1 value: " << metric1.Get() << std::endl;
  std::cout << " - metric2 value: " << metric2.Get() << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// Legacy API examples
// =============================================================================

// =============================================================================
// test_legacy_7 - Legacy prometheus-cpp-lite API: full explicit chain
//
// Demonstrates the most verbose construction pattern with separate integer
// and floating-point gauge families registered in a user-owned registry.
// =============================================================================

void test_legacy_7() {
  std::cout << "\n=== test_legacy_7 - Legacy prometheus-cpp-lite API: full explicit chain ===\n";

  using IntegerGauge  = Gauge<int64_t>;
  using FloatingGauge = Gauge<double>;

  using IntegerGaugeFamily  = CustomFamily<IntegerGauge>;
  using FloatingGaugeFamily = CustomFamily<FloatingGauge>;

  Registry registry;

  // Integer gauge family.
  IntegerGaugeFamily& gaugeFamily1 {
    IntegerGauge::Family::Build(registry, "gauge_family_1", "gauge for check integer functionality", {{"type", "integer"}})
  };

  IntegerGauge& gauge11 { gaugeFamily1.Add({{"number", "1"}}) };
  IntegerGauge& gauge12 { gaugeFamily1.Add({{"number", "2"}}) };

  // Floating-point gauge family.
  FloatingGaugeFamily& gaugeFamily2 {
    FloatingGauge::Family::Build(registry, "gauge_family_2", "gauge for check floating functionality", {{"type", "float"}})
  };

  FloatingGauge& gauge21 { gaugeFamily2.Add({{"number", "1"}}) };
  FloatingGauge& gauge22 { gaugeFamily2.Add({{"number", "2"}}) };

  gauge11.Set(100);
  gauge11.Decrement(25);

  gauge12 += 50;
  gauge12--;

  gauge21.Set(3.14);
  gauge21 -= 1.0;

  gauge22.Set(99.9);
  gauge22.Decrement(0.9);

  std::cout << " - Gauge11 value: " << gauge11.Get() << std::endl;
  std::cout << " - Gauge12 value: " << gauge12.Get() << std::endl;
  std::cout << " - Gauge21 value: " << gauge21.Get() << std::endl;
  std::cout << " - Gauge22 value: " << gauge22.Get() << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// main
// =============================================================================

int main() {
  std::cout << "=== Gauge metric - all usage variants ===\n";

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
    test_legacy_7();
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}