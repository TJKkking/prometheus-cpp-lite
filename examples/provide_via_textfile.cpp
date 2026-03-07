/*
* prometheus-cpp-lite — header-only C++ library for exposing Prometheus metrics
* https://github.com/biaks/prometheus-cpp-lite
*
* Copyright (c) 2026 Yan Kryukov ianiskr@gmail.com
* Licensed under the MIT License
*
* =============================================================================
* provide_via_textfile.cpp — File export example
*
* Demonstrates shortest way to save metrics to a .prom file via file_saver_t,
* suitable for node_exporter textfile collector
*
*/

#include <prometheus/counter.h>
#include <prometheus/file_saver.h>

#include <chrono>
#include <cstdlib>

using namespace prometheus;

int main() {

  registry_t       registry;
  counter_metric_t metric    (registry, "metric1_name", "description1");
  file_saver_t     filesaver (registry, std::chrono::seconds(5), "./metrics.prom");

  // now our metrics will be save every 5 seconds to ./metrics.prom local file

  for (;; ) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    const int random_value = std::rand();
    metric += random_value % 10;
  }

}

