/*
* prometheus-cpp-lite — header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2025 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* add_custom_metric_class.cpp — Custom metric class example
*
* Demonstrates how to create your own metric type by following the same pattern
* used by the built-in counter, gauge, histogram, etc.
*
* This example implements a "min_max_t" metric that tracks the minimum and
* maximum observed values — something not provided out of the box.
*
* Test list
* ---------
* test_main_1 — Direct usage with a user-owned registry and implicit family.
* test_main_2 — Usage with an explicit untyped family wrapper.
* test_main_3 — Usage with a typed family wrapper (compile-time type safety).
* test_main_4 — Usage with the global registry (shortest form).
* test_main_5 — Usage via the legacy prometheus-cpp Builder API.
*/

#include "prometheus/core.h"

#include <algorithm>
#include <limits>

using namespace prometheus;

// =============================================================================
// Global registry definition
// =============================================================================

namespace prometheus {
  registry_t global_registry;
}

// =============================================================================
// min_max_t — custom Prometheus metric that tracks minimum and maximum values
//
// Supports the standard two ownership modes:
//
//   min_max_t<double>   — owning:    holds its own atomic storage.
//   min_max_t<double&>  — reference: binds to the storage of an owning instance.
//
// Serialization produces two lines per metric:
//   name_min{labels} <min_value>
//   name_max{labels} <max_value>
// =============================================================================

template <typename MetricValue = double>
class min_max_t : public Metric {

public:
  using value_type   = typename atomic_storage<MetricValue>::value_type;
  using storage_type = typename atomic_storage<MetricValue>::storage_type;
  using Family       = CustomFamily<min_max_t<value_type>>;

private:
  storage_type current_min;
  storage_type current_max;
  storage_type count;

  value_type snapshot_min   = 0;
  value_type snapshot_max   = 0;
  value_type snapshot_count = 0;

  friend min_max_t<value_type&>;

public:

  // --- Owning constructor (min_max_t<value_type>) -----------------------------

  /// @brief Constructs an owning min_max metric with initial state "no observations".
  /// @param labels Per-metric dimensional labels.
  template <typename U = MetricValue, std::enable_if_t<!std::is_reference<U>::value, int> = 0>
  explicit min_max_t(const labels_t& labels)
    : Metric(labels)
    , current_min(std::numeric_limits<value_type>::max())
    , current_max(std::numeric_limits<value_type>::lowest())
    , count(0) {}

  // --- Reference constructors (min_max_t<value_type&>) ------------------------

  /// @brief Constructs a reference metric that binds to an existing owning metric.
  template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
  min_max_t(min_max_t<value_type>& other)
    : Metric(other.labels_ptr) , current_min(other.current_min), current_max(other.current_max), count(other.count) {}

  /// @brief Constructs a reference metric by adding an owning metric to an untyped family.
  template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
  min_max_t(family_t& family, const labels_t& labels = {})
    : min_max_t(family.Add<min_max_t<value_type>>(labels)) {}

  /// @brief Constructs a reference metric by adding an owning metric to a typed family.
  template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
  min_max_t(custom_family_t<min_max_t<value_type>>& family, const labels_t& labels = {})
    : min_max_t(family.Add(labels)) {}

  /// @brief Constructs a reference metric, creating both family and metric in the given registry.
  template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
  min_max_t(Registry& registry, const std::string& name, const std::string& help, const labels_t& labels = {})
    : min_max_t(registry.Add(name, help, labels).Add<min_max_t<value_type>>({})) {}

  /// @brief Constructs a reference metric using the global registry.
  template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
  min_max_t(const std::string& name, const std::string& help, const labels_t& labels = {})
    : min_max_t(global_registry.Add(name, help, labels).Add<min_max_t<value_type>>({})) {}

  // --- Non-copyable (owning form only) ----------------------------------------

  template <typename U = MetricValue, std::enable_if_t<!std::is_reference<U>::value, int> = 0>
  min_max_t(const min_max_t&) = delete;

  template <typename U = MetricValue, std::enable_if_t<!std::is_reference<U>::value, int> = 0>
  min_max_t& operator=(const min_max_t&) = delete;

  // --- Public API -------------------------------------------------------------

  /// @brief Records an observation, updating min and max accordingly.
  /// @param val Observed value.
  void Observe(value_type val) {
    // Update minimum using CAS loop.
    value_type prev_min = current_min.load();
    while (val < prev_min)
      if (current_min.compare_exchange_weak(prev_min, val))
        break;

    // Update maximum using CAS loop.
    value_type prev_max = current_max.load();
    while (val > prev_max)
      if (current_max.compare_exchange_weak(prev_max, val))
        break;

    ++count;
  }

  
  value_type GetMin()   const { return current_min.load(); } ///< @brief Returns the current minimum observed value.
  value_type GetMax()   const { return current_max.load(); } ///< @brief Returns the current maximum observed value.
  value_type GetCount() const { return count.load();       } ///< @brief Returns the total number of observations.

  // --- Metric interface overrides ---------------------------------------------

  /// @brief Returns the Prometheus type name for this metric.
  const char* type_name() const override { return "gauge"; }

  /// @brief Freezes values into snapshots for consistent serialization.
  void collect() override {
    snapshot_min   = current_min.load();
    snapshot_max   = current_max.load();
    snapshot_count = count.load();
  }

  /// @brief Writes this metric's data lines in Prometheus text exposition format.
  ///
  /// Produces three lines:
  ///   name_min{labels} <value>
  ///   name_max{labels} <value>
  ///   name_count{labels} <value>
  void serialize(std::ostream& out, const std::string& family_name, const labels_t& base_labels) const override {
    // Only serialize if at least one observation was made.
    if (snapshot_count > 0) {
      TextSerializer::WriteLine(out, family_name, base_labels, this->get_labels(), snapshot_min, "_min");
      TextSerializer::WriteLine(out, family_name, base_labels, this->get_labels(), snapshot_max, "_max");
    }
    TextSerializer::WriteLine(out, family_name, base_labels, this->get_labels(), snapshot_count, "_count");
  }
};

// =============================================================================
// Convenience aliases (following the same pattern as built-in metrics)
// =============================================================================

using min_max_metric_t = typename modify_t<min_max_t<>>::metric_ref; ///< @brief Zero-copy reference handle to a min_max metric.
using min_max_family_t = custom_family_t<min_max_t<>>;               ///< @brief Typed family alias for min_max metrics.
using BuildMinMax      = Builder<min_max_t<double>>;                 ///< @brief Fluent builder alias for the min_max metric.

// =============================================================================
// test_main_1 — Direct usage with a user-owned registry and implicit family
//
// Chain: registry → (implicit family) → metric
// =============================================================================

void test_main_1() {
  std::cout << "\n=== test_main_1 — User-owned registry, implicit family ===\n";

  registry_t       registry;
  min_max_metric_t temperature (registry, "room_temperature_celsius", "Room temperature range");

  temperature.Observe(21.5);
  temperature.Observe(18.3);
  temperature.Observe(25.7);
  temperature.Observe(22.0);

  std::cout << " - min:   " << temperature.GetMin()   << std::endl;
  std::cout << " - max:   " << temperature.GetMax()   << std::endl;
  std::cout << " - count: " << temperature.GetCount() << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_main_2 — Usage with an explicit untyped family wrapper
//
// Chain: registry → family → metric
// =============================================================================

void test_main_2() {
  std::cout << "\n=== test_main_2 — Explicit untyped family wrapper ===\n";

  registry_t       registry;
  family_t         sensors       (registry, "sensor_reading", "Sensor min/max readings", {{"location", "lab"}});
  min_max_metric_t humidity      (sensors, {{"type", "humidity"}});
  min_max_metric_t pressure      (sensors, {{"type", "pressure"}});

  humidity.Observe(45.0);
  humidity.Observe(62.3);
  humidity.Observe(38.1);

  pressure.Observe(1013.25);
  pressure.Observe(1008.50);
  pressure.Observe(1020.00);

  std::cout << " - humidity min:  " << humidity.GetMin()  << ", max: " << humidity.GetMax()  << std::endl;
  std::cout << " - pressure min:  " << pressure.GetMin()  << ", max: " << pressure.GetMax()  << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_main_3 — Usage with a typed family wrapper (compile-time type safety)
//
// Chain: registry → typed family → metric
// =============================================================================

void test_main_3() {
  std::cout << "\n=== test_main_3 — Typed family wrapper (compile-time safety) ===\n";

  registry_t       registry;
  min_max_family_t latency       (registry, "request_latency_ms", "Request latency range", {{"service", "api"}});
  min_max_metric_t latency_get   (latency, {{"method", "GET"}});
  min_max_metric_t latency_post  (latency, {{"method", "POST"}});

  // If you uncomment this, you'll get a compile-time type mismatch error:
  // counter_metric_t wrong (latency, {{"method", "DELETE"}});

  latency_get.Observe(12.5);
  latency_get.Observe(3.2);
  latency_get.Observe(45.8);

  latency_post.Observe(100.0);
  latency_post.Observe(55.3);
  latency_post.Observe(200.7);

  std::cout << " - GET  min: " << latency_get.GetMin()  << ", max: " << latency_get.GetMax()  << std::endl;
  std::cout << " - POST min: " << latency_post.GetMin() << ", max: " << latency_post.GetMax() << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// test_main_4 — Usage with the global registry (shortest form)
//
// Chain: global_registry → (implicit family) → metric
// =============================================================================

void test_main_4() {
  std::cout << "\n=== test_main_4 — Global registry (shortest form) ===\n";

  global_registry = Registry();  // Clear global registry for a clean test.

  min_max_metric_t cpu_freq ("cpu_frequency_mhz", "CPU frequency range");

  cpu_freq.Observe(2400.0);
  cpu_freq.Observe(3600.0);
  cpu_freq.Observe(800.0);
  cpu_freq.Observe(2800.0);

  std::cout << " - min:   " << cpu_freq.GetMin()   << std::endl;
  std::cout << " - max:   " << cpu_freq.GetMax()   << std::endl;
  std::cout << " - count: " << cpu_freq.GetCount() << std::endl;
  std::cout << " - output serialized data:\n" << global_registry.serialize();
}

// =============================================================================
// test_main_5 — Legacy prometheus-cpp Builder API
//
// Chain: Registry& → BuildMinMax().Register() → CustomFamily& → min_max_t&
// =============================================================================

void test_main_5() {
  std::cout << "\n=== test_main_5 — Legacy Builder API ===\n";

  using MinMax       = min_max_t<double>;
  using MinMaxFamily = CustomFamily<MinMax>;

  Registry     registry;
  MinMaxFamily& response_size = BuildMinMax().Name("response_size_bytes")
                                             .Help("Response size range")
                                             .Labels({{"service", "web"}})
                                             .Register(registry);

  MinMax& size_html = response_size.Add({{"content_type", "html"}});
  MinMax& size_json = response_size.Add({{"content_type", "json"}});

  size_html.Observe(1024);
  size_html.Observe(512);
  size_html.Observe(4096);

  size_json.Observe(256);
  size_json.Observe(128);
  size_json.Observe(2048);

  std::cout << " - html min: " << size_html.GetMin() << ", max: " << size_html.GetMax() << std::endl;
  std::cout << " - json min: " << size_json.GetMin() << ", max: " << size_json.GetMax() << std::endl;
  std::cout << " - output serialized data:\n" << registry.serialize();
}

// =============================================================================
// main
// =============================================================================

int main() {
  std::cout << "=== Custom metric class (min_max_t) — all usage variants ===\n";

  try {
    test_main_1();
    test_main_2();
    test_main_3();
    test_main_4();
    test_main_5();
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}