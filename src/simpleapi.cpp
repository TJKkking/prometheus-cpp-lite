
#include "prometheus/core.h"
#include "prometheus/file_saver.h"

#ifdef PROMETHEUS_USE_IP_SOCKETS
#include "prometheus/http_puller.h"
#include "prometheus/http_pusher.h"
#endif

namespace prometheus {

  Registry       global_registry;

  file_saver_t   file_saver  {global_registry};
  #ifdef PROMETHEUS_USE_IP_SOCKETS
  http_pusher_t  http_pusher {global_registry};
  http_server_t  http_server {global_registry};
  #endif

}
