
// Compatibility shim — prometheus-cpp.
// In v2.0 it functional in http_puller.h
#pragma once
#pragma message("warning: <prometheus/gateway.h> is deprecated in v2.0, use <prometheus/http_puller.h> instead")
#include "prometheus/http_puller.h"
