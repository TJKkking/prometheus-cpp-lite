/*
* prometheus-cpp-lite — header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* provide_via_http_pull_simple.cpp — HTTP pull endpoint examples
*
* Demonstrates shortest way to expose metrics via http_server_t,
* you can check result via command: "curl http://localhost:9100/metrics"
*
*/

#include <prometheus/counter.h>
#include <prometheus/http_puller.h>

using namespace prometheus;

int main () {

  registry_t       registry;
  counter_metric_t metric  (registry, "metric1_name", "description1");
  http_server_t    puller  (registry, "127.0.0.1:9100", "/metrics", log_e::info);

  // now our metrics always available at http://localhost:9100/metrics

  std::cout << "HTTP pull server started on http://localhost:9100/metrics\n"
            << "Use: curl http://localhost:9100/metrics or open in browser\n" << std::endl;

  for (;; ) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    metric += std::rand() % 10;
    std::cout << "  metric1_name = " << metric.Get() << std::endl;
  }

}
