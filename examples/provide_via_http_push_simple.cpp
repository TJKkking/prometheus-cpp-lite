
#include <prometheus/counter.h>
#include <prometheus/http_pusher.h>

#include <chrono>
#include <cstdlib>

using namespace prometheus;

int main() {

  registry_t       registry;
  counter_metric_t metric  (registry, "metric1_name", "description1");
  http_pusher_t    pusher  (registry, std::chrono::seconds(5), "http://localhost:9091/metrics/job/test");

  // now our metrics will be send every 5 seconds to http://localhost:9091/metrics/job/test
  // you can check it with server_side_for_http_push.py script

  for (;; ) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    const int random_value = std::rand();
    metric += random_value % 10;
  }

}
