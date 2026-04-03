
/*
* prometheus-cpp-lite — header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*/

#pragma once

#include "prometheus/core.h"

namespace prometheus {

  // =============================================================================
  // counter_t — Prometheus counter metric
  //
  // A counter is a cumulative metric that can only increase (or be reset to zero
  // on process restart).  Supports two ownership modes controlled by MetricValue:
  //
  //   counter_t<uint64_t>   — owning:    holds its own std::atomic<uint64_t>.
  //   counter_t<uint64_t&>  — reference: binds to the atomic of an owning counter.
  //
  // The reference form enables zero-copy metric handles (see make_ref<>).
  // =============================================================================

  /// @brief Prometheus counter metric — monotonically increasing value.
  ///
  /// @tparam MetricValue Value type. Use a plain type (e.g. `uint64_t`) for an
  ///         owning counter, or a reference type (e.g. `uint64_t&`) for a
  ///         zero-copy reference handle.
  ///
  /// Base class is selected automatically:
  ///   - owning form   inherits Metric          (vtable, labels, snapshot — for Family storage)
  ///   - reference form inherits metric_ref_base (labels_ptr only — 8 bytes, no vtable)
  template <typename MetricValue = uint64_t>
  class counter_t : public metric_base_for<MetricValue> {

    using base_t = metric_base_for<MetricValue>;

  public:
    using storage_type = typename atomic_storage<MetricValue>::storage_type;
    using value_type   = typename atomic_storage<MetricValue>::value_type;
    using Family       = CustomFamily<counter_t<value_type> >; ///< Legacy alias for backward compatibility.

  private:
    storage_type val;
    value_type   snapshot_value { 0 };

    friend counter_t<value_type&>; ///< Grant access to internals so the reference form can bind to the owning form.

  public:

    // --- Owning constructor (counter_t<value_type>) -----------------------------

    /// @brief Constructs an owning counter initialized to zero.
    /// @param labels Per-metric dimensional labels (copied and owned).
    template <typename U = MetricValue, std::enable_if_t<!std::is_reference<U>::value, int> = 0>
    explicit counter_t(const labels_t& labels)
      : base_t(labels), val(0) {}

    // --- SimpleAPI: easy to use from the user's side, non-trivial internally.
    // --- Reference constructors (counter_t<value_type&>) ------------------------

    /// @brief Default-constructs an unbound reference counter.
    ///        Must be reassigned via operator= before meaningful use.
    template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
    counter_t()
      : base_t(), val(null_atomic<value_type>()) {}

    /// @brief Constructs a reference counter that binds to an existing owning counter.
    /// @param other Owning counter whose atomic value and labels are referenced.
    template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
    counter_t(counter_t<value_type>& other)
      : base_t(other.labels_ptr), val(other.val) {}

    /// @brief Constructs a reference counter by adding an owning counter to the given family.
    /// The metric value type compatibility with the family is checked at runtime.
    /// @param family Family to add the owning counter to.
    /// @param labels Per-metric dimensional labels.
    template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
    counter_t(family_t& family, const labels_t& labels = {})
      : counter_t(family.Add<counter_t<value_type> >(labels)) {}

    /// @brief Constructs a reference counter by adding an owning counter to the given family.
    /// The metric value type compatibility with the family is enforced at compile time.
    /// @param family Family to add the owning counter to.
    /// @param labels Per-metric dimensional labels.
    template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
    counter_t(custom_family_t<counter_t<value_type> >& family, const labels_t& labels = {})
      : counter_t(family.Add (labels)) {}

    /// @brief Constructs a reference counter, creating both family and metric in the given registry.
    /// @param registry Registry to register the family in.
    /// @param name     Metric family name.
    /// @param help     Help/description string.
    /// @param labels   Constant base labels for the family.
    template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
    counter_t(Registry& registry, const std::string& name, const std::string& help = {}, const labels_t& labels = {})
      : counter_t(registry.Add(name, help).Add<counter_t<value_type> >(labels)) {}

    /// @brief Constructs a reference counter, creating both family and metric in the given registry.
    /// @param registry Shared pointer to Registry to register the family in.
    /// @param name     Metric family name.
    /// @param help     Help/description string.
    /// @param labels   Constant base labels for the family.
    template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
    counter_t(std::shared_ptr<registry_t>& registry, const std::string& name, const std::string& help = {}, const labels_t& labels = {})
      : counter_t(registry->Add(name, help).Add<counter_t<value_type> >(labels)) {}

    /// @brief Constructs a reference counter using the global registry.
    /// @param name   Metric family name.
    /// @param help   Help/description string.
    /// @param labels Constant base labels for the family.
    template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
    counter_t(const std::string& name, const std::string& help, const labels_t& labels = {})
      : counter_t(global_registry.Add(name, help).Add<counter_t<value_type> >(labels)) {}

    /// @brief Constructs a reference counter using the global registry.
    /// @param name   Metric family name.
    /// @param labels Constant base labels for the family.
    template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
    counter_t(const std::string& name, const labels_t& labels = {})
      : counter_t(global_registry.Add(name).Add<counter_t<value_type> >(labels)) {}

    // --- Non-copyable (owning form only) ----------------------------------------

    /// @brief Owning counters are non-copyable to prevent accidental duplication.
    template <typename U = MetricValue, std::enable_if_t<!std::is_reference<U>::value, int> = 0>
    counter_t(const counter_t&) = delete;

    /// @brief Owning counters are non-copy-assignable.
    template <typename U = MetricValue, std::enable_if_t<!std::is_reference<U>::value, int> = 0>
    counter_t& operator=(const counter_t&) = delete;

    // --- Reference form: copy/move constructible and assignable ------------------

    /// @brief Reference counters are copy-constructible (rebinds to the same atomic).
    template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
    counter_t(const counter_t& other)
      : base_t(other.labels_ptr), val(other.val) {}

    /// @brief Reference counters are move-constructible.
    template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
    counter_t(counter_t&& other)
      : base_t(other.labels_ptr), val(other.val) {}

    /// @brief Reference counters support copy-assignment by rebinding via placement new.
    template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
    counter_t& operator=(const counter_t& other) {
      if (this != &other) {
        this->~counter_t();
        new (this) counter_t(other);
      }
      return *this;
    }

    /// @brief Reference counters support move-assignment by rebinding via placement new.
    template <typename U = MetricValue, std::enable_if_t<std::is_reference<U>::value, int> = 0>
    counter_t& operator=(counter_t&& other) {
      if (this != &other) {
        this->~counter_t();
        new (this) counter_t(std::move(other));
      }
      return *this;
    }

    // --- Public API (shared by both owning and reference forms) -----------------

    /// @brief Increments the counter by one.
    void Increment() { ++val; }

    /// @brief Increments the counter by the given non-negative value.
    ///
    /// In debug builds, throws if `val` is negative (for signed integral types).
    /// Zero and negative values are silently ignored in release builds.
    ///
    /// @param value Amount to add (must be > 0).
    /// @throws std::invalid_argument (debug only) if value is negative and integral.
    void Increment(const value_type& value) {
      #ifndef NDEBUG
      if (std::is_integral<value_type>::value && value < 0)
        throw std::invalid_argument("counter increment must be non-negative");
      #endif
      if (value > 0)
        val += value;
    }

    /// @brief Returns the current counter value.
    /// @return Atomically loaded counter value.
    value_type Get() const { return val.load(); }

    /// @brief Returns the current counter value.
    /// @return Atomically loaded counter value.
    value_type value() const { return val.load(); }

    /// @brief Pre-increment operator (increments by one).
    /// @return Reference to this counter.
    counter_t& operator++()                    { ++val;      return *this; }

    /// @brief Post-increment operator (increments by one, returns reference — not previous value).
    /// @return Reference to this counter.
    counter_t& operator++(int)                 { ++val;      return *this; }

    /// @brief Compound addition operator.
    /// @param v Value to add.
    /// @return Reference to this counter.
    counter_t& operator+=(const value_type& v) { Increment(v); return *this; }

    // --- Metric interface overrides (owning form only) --------------------------
    //
    // These are plain (non-template) methods. For the owning form, base_t = Metric,
    // so they override the pure virtual methods. For the reference form,
    // base_t = metric_ref_base which has no virtuals, so these are just
    // regular unused methods that the compiler can eliminate.

    const char* type_name() const { return "counter"; }

    void collect() { snapshot_value = val.load(); }

    void serialize(std::ostream& out, const std::string& family_name, const labels_t& base_labels) const {
      TextSerializer::WriteLine(out, family_name, base_labels, this->get_labels(), snapshot_value);
    }
  };

  // =============================================================================
  // Convenience aliases
  // =============================================================================

  /// @brief Generic counter alias with configurable value type.
  template <typename MetricType = uint64_t>
  using Counter = counter_t<MetricType>;

  /// @brief Builder alias for the default counter type (legacy from prometheus-cpp, should by double).
  using BuildCounter = Builder<counter_t<double>>;

  /// @brief Zero-copy reference handle to a default counter (SimpleAPI).
  using counter_metric_t = typename modify_t<counter_t<>>::metric_ref;

  /// @brief Typed family alias for default counters (SimpleAPI).
  using counter_family_t = custom_family_t<counter_t<>>;

} // namespace prometheus
