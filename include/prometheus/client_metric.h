
// Compatibility shim — v1.0 header name.
// In v2.0 all core classes are in core.h.
#pragma once
#pragma message("warning: <prometheus/client_metric.h> is deprecated in v2.0, use <prometheus/core.h> instead")
#include "prometheus/core.h"
