/*
* prometheus-cpp-lite - header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* test_counter.cpp - Counter metric usage examples
*
* This file demonstrates every supported way to create and use Counter metrics
* with the prometheus-cpp-lite library — from the shortest one-liner forms
* that rely on the global registry, through explicit typed/untyped family
* wrappers, to the legacy APIs compatible with prometheus-cpp and
* prometheus-cpp-lite-core.
*
* Test list
* ---------
* test_main_1   — Shortest path: global registry + implicit family (uint64_t).
* test_main_2   — Shortest path with custom value type (double) and labels.
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

#include "prometheus/counter.h"

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
// The simplest way to create a counter.  A family is created automatically
// behind the scenes inside the global registry. The default value type for
// counter metric is uint64_t.
// =============================================================================

void test_main_1() {
  std::cout << "\n=== test_main_1 - Shortest path with global registry and implicit family ===\n";

  counter_metric_t metric1 ("metric1_name", "description of metric1");
  counter_metric_t metric2 ("metric2_name", "description of metric2");

  metric1++;
  metric2 += 10;

  std::cout << " - metric1 value: " << metric1.Get() << std::endl;
  std::cout << " - metric2 value: " << metric2.Get() << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_main_2 - Shortest path with a custom value type (double) and labels
//
// Chain: global_registry → (implicit family) → metric
//
// Same as test_main_1, but demonstrate how to use counter_t<double&> to store
// a floating-point value instead of the default uint64_t.
// =============================================================================

void test_main_2() {
  std::cout << "\n=== test_main_2 - Shortest path with a custom value type (double) and labels ===\n";

  counter_t<double&> metric_load    ("duration_sec", "Duration in seconds", {{"CPU", "1"}, {"method", "load"}});
  counter_t<double&> metric_execute ("duration_sec", "Duration in seconds", {{"CPU", "1"}, {"method", "execute"}});

  metric_load++;
  metric_execute += 0.5;

  std::cout << " - metric_load    value: " <<    metric_load.Get() << std::endl;
  std::cout << " - metric_execute value: " << metric_execute.Get() << std::endl;
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

  family_t         requests    ("requests", "HTTP requests", {{"host", "localhost"}});
  counter_metric_t metric_get  (requests, {{"method",  "GET"}});
  counter_metric_t metric_post (requests, {{"method", "POST"}});

  // If you uncomment this, you will get a runtime exception because the check will happen at runtime:
  // ( double type of value not allowed in a family of counter_t<uint64_t> )
  //counter_t<double&> metric_execute (requests, {{"CPU", "1"}, {"method", "execute"}});

  metric_get++;
  metric_post += 10;

  std::cout << " - metric_get  value: " <<  metric_get.Get() << std::endl;
  std::cout << " - metric_post value: " << metric_post.Get() << std::endl;
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

  counter_family_t requests      ("requests", "HTTP requests");
  //                ^-- it the same family as in test_main_3, because it has the same name and store in same global_registry
  counter_metric_t metric_put    (requests, {{"method",    "PUT"}});
  counter_metric_t metric_delete (requests, {{"method", "DELETE"}});

  // If you uncomment this, you'll get a type mismatch error right in the IDE, because the check will happen at compile time:
  // ( uint64_t type of value not allowed in a family of counter_t<double> )
  //counter_t<double&> metric_execute (requests, {{"CPU", "1"}, {"method", "execute"}});

  metric_put++;
  metric_delete += 10;

  std::cout << " - metric_put    value: " <<    metric_put.Get() << std::endl;
  std::cout << " - metric_delete value: " << metric_delete.Get() << std::endl;
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

  registry_t       registry;
  counter_metric_t metric1 (registry, "metric1_name", "description1");
  counter_metric_t metric2 (registry, "metric2_name", "description2");

  metric1++;
  metric2 += 10;

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

  registry_t       registry;
  family_t         requests    (registry, "requests", "description", {{"host", "localhost"}});
  counter_metric_t metric_get  (requests, {{"CPU", "1"}, {"method",  "GET"}});
  counter_metric_t metric_post (requests, {{"CPU", "1"}, {"method", "POST"}});

  // If you uncomment this, you will get a runtime inconsistency exception because the check will happen at runtime:
  // ( double type of value not allowed in a family of counter_t<uint64_t> )
  //counter_t<double&> metric_execute (requests, {{"CPU", "1"}, {"method", "execute"}});

  metric_get++;
  metric_post += 10;

  std::cout << " - metric_get  value: " <<  metric_get.Get() << std::endl;
  std::cout << " - metric_post value: " << metric_post.Get() << std::endl;
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
  using counter_double_t = counter_t<double&>;
  using family_double_t  = custom_family_t<counter_t<double>>;

  std::shared_ptr<registry_t>      registry = std::make_shared<registry_t>();
  family_double_t  duration_sec   (registry, "duration_sec", "Duration in seconds", {{"host", "localhost"}});
  counter_double_t metric_load    (duration_sec, {{"CPU", "1"}, {"method", "load"}});
  counter_double_t metric_execute (duration_sec, {{"CPU", "1"}, {"method", "execute"}});

  // If you uncomment this, you'll get a type mismatch error right in the IDE, because the check will happen at compile time:
  // ( uint64_t type of value not allowed in a family of counter_t<double> )
  //counter_metric_t metric_post  (duration_sec, {{"CPU", "1"}, {"method", "POST"}});

  metric_load++;
  metric_execute += 10;

  std::cout << " - metric_load    value: " <<    metric_load.Get() << std::endl;
  std::cout << " - metric_execute value: " << metric_execute.Get() << std::endl;
  std::cout << " - output serialized data:\n" << registry->serialize();
}



// =============================================================================
// test_legacy_1 - Legacy prometheus-cpp like API: standalone Builder + references
//
// Chain: Registry& → BuildCounter().Register() → CustomFamily& → Counter&
//
// Uses the fluent Builder to construct and register a CustomFamily, then
// obtains raw Counter references.  This mirrors the prometheus-cpp style.
// =============================================================================

void test_legacy_1() {
  std::cout << "\n=== test_legacy_1 - Legacy prometheus-cpp like API: standalone Builder + references ===\n";

  typedef Counter<double>             DoubleCounter;
  typedef CustomFamily<DoubleCounter> MetricFamily;

  Registry       registry;
  MetricFamily&  duration_sec = BuildCounter().Name("duration_sec")
                                              .Help("description")
                                              .Labels({{"host", "localhost"}, {"legacy", "yes"}})
                                              .Register(registry);

  DoubleCounter& metric_load    = duration_sec.Add({{"CPU", "1"}, {"method", "load"}});
  DoubleCounter& metric_execute = duration_sec.Add({{"CPU", "1"}, {"method", "execute"}});

  metric_load++;
  metric_execute += 0.5;

  std::cout << " - metric_load    value: " <<    metric_load.Get() << std::endl;
  std::cout << " - metric_execute value: " << metric_execute.Get() << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_2 - Legacy prometheus-cpp-lite-core API: CustomFamily::Build() static method
//
// Chain: Registry& → CustomFamily::Build() → CustomFamily& → Counter&
//
// Same as test_legacy_1 but uses the static Build() method on CustomFamily instead
// of the standalone Builder.
// =============================================================================

void test_legacy_2() {
  std::cout << "\n=== test_legacy_2 - Legacy prometheus-cpp-lite-core API: CustomFamily::Build() static method ===\n";

  typedef Counter<double>             DoubleCounter;
  typedef CustomFamily<DoubleCounter> MetricFamily;

  Registry       registry;
  MetricFamily&  duration_sec   = MetricFamily::Build(registry, "duration_sec", "description", {{"host", "localhost"}});
  DoubleCounter& metric_load    = duration_sec.Add({{"CPU", "1"}, {"method", "load"}});
  DoubleCounter& metric_execute = duration_sec.Add({{"CPU", "1"}, {"method", "execute"}});

  metric_load++;
  metric_execute += 0.5;

  std::cout << " - metric_load    value: " <<    metric_load.Get() << std::endl;
  std::cout << " - metric_execute value: " << metric_execute.Get() << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_3 - Legacy prometheus-cpp-lite-core API: Registry::Add<MetricType>() typed registration
//
// Chain: Registry& → Registry::Add<Counter>() → CustomFamily& → Counter&
//
// Registers a typed family directly on the registry.  The family remembers
// the metric type so Add() does not need an explicit template argument.
// =============================================================================

void test_legacy_3() {
  std::cout << "\n=== test_legacy_3 - Legacy prometheus-cpp-lite-core API: Registry::Add<MetricType>() typed registration ===\n";

  typedef Counter<uint64_t>            IntegerCounter;
  typedef CustomFamily<IntegerCounter> MetricFamily;

  Registry        registry;
  MetricFamily&   requests    = registry.Add<IntegerCounter>("requests", "description", {{"host", "localhost"}});
  IntegerCounter& metric_get  = requests.Add({{"CPU", "1"}, {"method", "GET"}});
  IntegerCounter& metric_post = requests.Add({{"CPU", "1"}, {"method", "POST"}});

  metric_get.Increment();
  metric_post.Increment(10);

  std::cout << " - metric_get  value: " <<  metric_get.Get() << std::endl;
  std::cout << " - metric_post value: " << metric_post.Get() << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_legacy_4 - Legacy prometheus-cpp-lite-core API: untyped Family + explicit Family::Add<MetricType>()
//
// Chain: Registry& → Registry::Add() → Family& → Family::Add<Counter>()
//
// The most explicit form: the family is untyped, and every Add() call
// must specify the concrete metric type as a template argument.
// =============================================================================

void test_legacy_4() {
  std::cout << "\n=== test_legacy_4 - Legacy prometheus-cpp-lite-core API: untyped Family + explicit Family::Add<MetricType>() ===\n";

  Registry         registry;
  Family&          duration       = registry.Add("duration_sec", "description", {{"host", "localhost"}});
  Counter<double>& metric_load    = duration.Add<Counter<double>>({{"CPU", "1"}, {"method", "load"}});
  Counter<double>& metric_execute = duration.Add<Counter<double>>({{"CPU", "1"}, {"method", "execute"}});

  metric_load++;
  metric_execute += 0.5;

  std::cout << " - metric_load    value: " <<    metric_load.Get() << std::endl;
  std::cout << " - metric_execute value: " << metric_execute.Get() << std::endl;
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

  simpleapi::counter_metric_t metric1 { "metric1", "metric_description" };
  simpleapi::counter_metric_t metric2 { "metric2", "metric_description" };

  metric1++;
  metric2 += 10;

  std::cout << " - metric1 value: " << metric1.Get() << std::endl;
  std::cout << " - metric2 value: " << metric2.Get() << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_legacy_6 - Legacy SimpleAPI: family wrapper + metric wrappers
//
// Chain: global_registry → simpleapi::counter_family_t → simpleapi::counter_metric_t
//
// Uses the simpleapi namespace aliases and family.Add() to create metrics.
// =============================================================================

void test_legacy_6() {
  std::cout << "\n=== ttest_legacy_6 - Legacy SimpleAPI: family wrapper + metric wrappers ===\n";

  global_registry.RemoveAll();  // Clear global registry for a clean test.

  simpleapi::counter_family_t metric_family { "simple_family", "simple family example"  };
  simpleapi::counter_metric_t metric1       { metric_family.Add({{"name", "counter1"}}) };
  simpleapi::counter_metric_t metric2       { metric_family.Add({{"name", "counter2"}}) };

  metric1++;
  metric2++;

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
// and floating-point counter families registered in a user-owned registry.
// =============================================================================

void test_legacy_7() {
  std::cout << "\n=== test_legacy_7 - Legacy prometheus-cpp-lite API: full explicit chain ===\n";

  using IntegerCounter  = Counter<uint64_t>;
  using FloatingCounter = Counter<double>;

  using IntegerCounterFamily  = CustomFamily<IntegerCounter>;
  using FloatingCounterFamily = CustomFamily<FloatingCounter>;

  Registry registry;

  // Integer counter family.
  IntegerCounterFamily& counterFamily1 {
    IntegerCounter::Family::Build(registry, "counter_family_1", "counter for check integer functionality", {{"type", "integer"}})
  };

  IntegerCounter& counter11 { counterFamily1.Add({{"number", "1"}}) };
  IntegerCounter& counter12 { counterFamily1.Add({{"number", "2"}}) };

  // Floating-point counter family.
  FloatingCounterFamily& counterFamily2 {
    FloatingCounter::Family::Build(registry, "counter_family_2", "counter for check floating functionality", {{"type", "float"}})
  };

  FloatingCounter& counter21 { counterFamily2.Add({{"number", "1"}}) };
  FloatingCounter& counter22 { counterFamily2.Add({{"number", "2"}}) };

  std::cout << " - Counter1 value: " << counter11.Get() << std::endl;
  std::cout << " - Counter2 value: " << counter12.Get() << std::endl;
  std::cout << " - Counter1 value: " << counter21.Get() << std::endl;
  std::cout << " - Counter2 value: " << counter22.Get() << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// main
// =============================================================================

int main() {
  std::cout << "=== Counter metric - all usage variants ===\n";

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