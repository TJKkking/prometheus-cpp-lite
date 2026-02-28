
#include <prometheus/prometheus.h>

using namespace prometheus;

int main() {

  counter_metric_t   requests  ("http_requests_total",      "Total requests");
  gauge_metric_t     active    ("active_connections",       "Open connections");
  histogram_metric_t latency   ("request_duration_seconds", "Request latency");
  summary_metric_t   response  ("response_time_seconds",    "Response time");
  benchmark_metric_t uptime    ("uptime_seconds",           "Process uptime");

  file_saver. start(std::chrono::seconds(5), "./metrics.txt");
  http_server.start("127.0.0.1:9091");
  http_pusher.start(std::chrono::seconds(5), "http://localhost:9091/metrics/job/test");

  uptime.start();
  for (int i = 0; i < 60; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    requests++;
    active.Set       (10    +  std::rand() % 50);
    latency.Observe  (0.001 * (std::rand() % 1000));
    response.Observe (0.001 * (std::rand() % 500));
    if (i % 10 == 0)
      std::cout << global_registry.serialize() << std::endl;
  }
  uptime.stop();
}
