
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

