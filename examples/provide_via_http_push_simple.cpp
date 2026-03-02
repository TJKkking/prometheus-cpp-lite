/*
* prometheus-cpp-lite — header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* provide_via_http_push_simple.cpp — HTTP push endpoint examples
*
* Demonstrates shortest way to push metrics via http_pusher_t,
* you can check it with server_side_for_http_push.py script
*
*/

#include <prometheus/counter.h>
#include <prometheus/http_pusher.h>

#include <chrono>
#include <cstdlib>

using namespace prometheus;

int main () {

  registry_t       registry;
  counter_metric_t metric  (registry, "metric1_name", "description1");
  http_pusher_t    pusher  (registry, std::chrono::seconds(5), "http://localhost:9091/metrics/job/test", log_e::info);

  // now our metrics will be send every 5 seconds to http://localhost:9091/metrics/job/test
  // you can check it with server_side_for_http_push.py script

  std::cout << "HTTP push started, sending every 5 seconds to http://localhost:9091/metrics/job/test\n"
            << "Run: python server_side_for_http_push.py\n" << std::endl;

  for (;; ) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    metric += std::rand() % 10;
    std::cout << "  metric1_name = " << metric.Get() << std::endl;
  }

}
