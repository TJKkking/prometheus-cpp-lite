#include "prometheus/simpleapi.h"

#include <memory>

namespace prometheus {
  namespace simpleapi {

    std::shared_ptr<Registry> registry_ptr = std::make_shared<Registry>();
    Registry&                 registry = *registry_ptr;
    SaveToFile saver(registry_ptr, std::chrono::seconds(10), std::string("./metrics.prom"));
    Family::Labels            prometheus::simpleapi::counter_metric_t::label = Family::Labels ();
    PushToServer pusher (registry_ptr, std::chrono::seconds (10), std::string ("http://localhost:9091/metrics"));
  }
}
