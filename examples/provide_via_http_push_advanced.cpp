/*
* prometheus-cpp-lite — header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* provide_via_http_push_advanced.cpp — HTTP push endpoint examples
*
* Demonstrates multiple ways to push metrics via http_pusher_t:
*
*   example_periodic()   — Background thread pushes metrics every N seconds.
*   example_on_demand()  — Manual Push/PushAdd/Delete calls (Gateway-compatible API).
*
* Both examples push to http://localhost:9091.
* Use server_side_for_http_push.py as the receiving end:
*
*   python server_side_for_http_push.py
*
* Then run this example and observe the requests in the Python console.
*/

#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/http_pusher.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace prometheus;

// =============================================================================
// Example 1 — Periodic push mode
//
// The most common scenario for long-running jobs: a background thread
// pushes the entire registry to the Pushgateway every 5 seconds.
//
// This is equivalent to the simple example, but with multiple metric types
// and console output showing what's happening.
//
//   python server_side_for_http_push.py
// =============================================================================

void example_periodic () {
  std::cout   << "\n=== Example 1: Periodic push — background thread every 5 seconds ===\n";
  std::cout   << "  Pushing to http://localhost:9091/metrics/job/periodic_example for 15 seconds...\n\n";

  // 1. Create a shared registry.
  auto registry = std::make_shared<registry_t>();

  // 2. Create metrics inside it.
  counter_metric_t requests (registry, "http_requests_total", "Total HTTP requests");
  gauge_metric_t   active   (registry, "active_connections",  "Currently open connections");

  // 3. Start the periodic pusher — metrics are now pushed every 5 seconds.
  http_pusher_t    pusher   (registry, std::chrono::seconds(5), "http://localhost:9091/metrics/job/periodic_example", log_e::info);

  // Simulate application work.
  for (int i = 0; i < 15; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    requests += 1 + std::rand() % 10;
    active    = 5 + std::rand() % 20;

    std::cout << "  [periodic] tick " << (i + 1) << "/15"
              << "  requests=" << requests.Get()
              << "  active="   << active.Get() << std::endl;
  }

  // Pusher stops automatically when it goes out of scope.
  std::cout   << "  Stopped.\n";
}

// =============================================================================
// Example 2 — On-demand push mode (Gateway-compatible API)
//
// Useful for short-lived batch jobs: compute metrics, push once, exit.
// Demonstrates the three Pushgateway operations:
//
//   Push()    — POST:   replaces ALL metrics for this job on the Pushgateway.
//   PushAdd() — PUT:    updates only the sent metrics, preserves others.
//   Delete()  — DELETE: removes all metrics for this job from the Pushgateway.
//
// This API is compatible with the prometheus-cpp Gateway class.
//
//   python server_side_for_http_push.py
// =============================================================================

void example_on_demand () {
  std::cout << "\n=== Example 2: On-demand push — Gateway-compatible API ===\n";
  std::cout << "  Pushing to http://localhost:9091 with job=batch_job, instance=host1\n\n";

  // 1. Create the pusher using the Gateway-compatible constructor.
  //    URI will be: /metrics/job/batch_job/instance/host1
  http_pusher_t gateway ("localhost", "9091", "batch_job", {{"instance", "host1"}}, log_e::info);

  // 2. Create a registry and register it.
  auto registry = std::make_shared<registry_t>();
  gateway.RegisterCollectable(registry);

  // 3. Create metrics.
  counter_metric_t items_processed (registry, "items_processed_total", "Total items processed");
  gauge_metric_t   last_batch_size (registry, "last_batch_size",       "Size of the last batch");

  // --- Step A: Simulate a batch job and Push (POST) all metrics. ---
  std::cout << "  Step A: Processing batch and pushing via POST...\n";
  for (int i = 0; i < 5; ++i) {
    items_processed += 10 + std::rand() % 20;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  last_batch_size = 42;

  int status = gateway.Push();
  std::cout << "  Push() returned HTTP status: " << status
            << "  items="      << items_processed.Get()
            << "  batch_size=" << last_batch_size.Get() << "\n\n";

  // --- Step B: Process more items and PushAdd (PUT) — updates without replacing. ---
  std::cout << "  Step B: Processing more items and pushing via PUT (PushAdd)...\n";
  for (int i = 0; i < 3; ++i) {
    items_processed += 5 + std::rand() % 10;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  last_batch_size = 99;

  status = gateway.PushAdd();
  std::cout << "  PushAdd() returned HTTP status: " << status
            << "  items=" << items_processed.Get()
            << "  batch_size=" << last_batch_size.Get() << "\n\n";

  // --- Step C: Clean up — delete metrics from the Pushgateway. ---
  std::cout << "  Step C: Deleting metrics from the Pushgateway...\n";
  status = gateway.Delete();
  std::cout << "  Delete() returned HTTP status: " << status << "\n\n";

  std::cout << "  Done.\n";
}

// =============================================================================
// Example 3 — Async push
//
// Same as on-demand, but non-blocking.  Useful when you don't want to wait
// for the network round-trip.
// =============================================================================

void example_async () {
  std::cout << "\n=== Example 3: Async push — non-blocking Gateway API ===\n";
  std::cout << "  Pushing to http://localhost:9091 with job=async_job\n\n";

  http_pusher_t gateway ("localhost", "9091", "async_job", {}, log_e::info);

  auto registry = std::make_shared<registry_t>();
  gateway.RegisterCollectable(registry);

  counter_metric_t events (registry, "events_total", "Total events");
  events += 100;

  // Fire-and-forget: the push happens in a background thread.
  std::cout << "  Launching AsyncPush()...\n";
  std::future<int> future = gateway.AsyncPush();

  // Do other work while the push is in progress.
  std::cout << "  Doing other work while push is in progress...\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Wait for the result when convenient.
  int status = future.get();
  std::cout << "  AsyncPush() returned HTTP status: " << status << "\n\n";

  // Async delete.
  std::cout << "  Launching AsyncDelete()...\n";
  future = gateway.AsyncDelete();
  status = future.get();
  std::cout << "  AsyncDelete() returned HTTP status: " << status << "\n\n";

  std::cout << "  Done.\n";
}

// =============================================================================
// main
// =============================================================================

int main () {
  std::cout << "=== HTTP Push endpoint examples ===\n";
  std::cout << "Make sure server_side_for_http_push.py is running:\n";
  std::cout << "  python server_side_for_http_push.py\n";

  example_periodic();
  example_on_demand();
  example_async();

  return 0;
}