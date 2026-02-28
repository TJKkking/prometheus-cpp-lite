
// Compatibility shim — v1.0 header name.
// In v2.0 it functional in http_pusher.h
#pragma once
#pragma message("warning: <prometheus/gateway.h> is deprecated in v2.0, use <prometheus/http_pusher.h> instead")
#include "prometheus/http_pusher.h"
