
// Compatibility shim — v1.0 header name.
// In v2.0 it functional in file_saver.h
#pragma once
#pragma message("warning: <prometheus/save_to_file.h> is deprecated in v2.0, use <prometheus/file_saver.h> instead")
#include "prometheus/file_saver.h"
