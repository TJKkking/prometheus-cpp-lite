
cmake_minimum_required(VERSION 3.12)
project(prometheus-cpp-lite)

# =============================================================================
# Options
# =============================================================================

option(PROMETHEUS_BUILD_EXAMPLES                "Build with examples"    OFF)
option(PROMETHEUS_USE_IP_SOCKETS                "Use HTTP push and pull" ON)

# =============================================================================
# Output directories
# =============================================================================

# for initialise variables: CMAKE_INSTALL_INCLUDEDIR, CMAKE_INSTALL_LIBDIR, CMAKE_INSTALL_BINDIR	
include(GNUInstallDirs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

if(WIN32)
  foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/bin)
  endforeach()
endif()

# =============================================================================
# Collect header files
# =============================================================================
# include all header files from header directory
#file(GLOB PROMETHEUS_CPP_LITE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/prometheus/*.h")
# manual include header files
file(GLOB PROMETHEUS_CORE_HEADERS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/prometheus/core.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/prometheus/atomic_floating.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/prometheus/counter.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/prometheus/gauge.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/prometheus/histogram.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/prometheus/summary.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/prometheus/benchmark.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/prometheus/info.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/prometheus/file_saver.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/prometheus/simpleapi.h"
)

if(PROMETHEUS_USE_IP_SOCKETS)
  file(GLOB PROMETHEUS_NETWORK_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/prometheus/http_puller.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/prometheus/http_pusher.h"
  )
endif()

set(PROMETHEUS_CPP_LITE_HEADERS ${PROMETHEUS_CORE_HEADERS} ${PROMETHEUS_NETWORK_HEADERS})

# =============================================================================
# Primary target: prometheus-cpp-lite (always header-only)
# =============================================================================

add_library               (${PROJECT_NAME} INTERFACE)
target_sources            (${PROJECT_NAME} INTERFACE ${PROMETHEUS_CPP_LITE_HEADERS})
target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:  ${CMAKE_CURRENT_SOURCE_DIR}/include>
                                                     $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# =============================================================================
# Secondary target: prometheus-cpp-lite-simpleapi (static, with global objects)
# =============================================================================

add_library               (${PROJECT_NAME}-full STATIC "src/simpleapi.cpp" "include/prometheus/prometheus.h" "include/prometheus/exposer.h")
target_link_libraries     (${PROJECT_NAME}-full PUBLIC ${PROJECT_NAME})

# =============================================================================
# Legacy aliases (v1.0 compatibility)
# =============================================================================

if(NOT TARGET prometheus-cpp-lite-core)
  add_library(prometheus-cpp-lite-core ALIAS ${PROJECT_NAME})
endif()

if(NOT TARGET prometheus-cpp-simpleapi)
  add_library(prometheus-cpp-simpleapi ALIAS ${PROJECT_NAME}-full)
endif()

# =============================================================================
# Third-party dependencies (must come before main target so targets exist)
# =============================================================================

# it need for http_pusher_t, http_puller_t
if(PROMETHEUS_USE_IP_SOCKETS)
  add_subdirectory("3rdparty/ip-sockets-cpp-lite")
  target_link_libraries     (${PROJECT_NAME} INTERFACE ip-sockets-cpp-lite)
  target_compile_definitions(${PROJECT_NAME} INTERFACE PROMETHEUS_USE_IP_SOCKETS=1)
endif()

# it need for file_saver_t, http_pusher_t, http_puller_t
if(NOT WIN32)
  find_package(Threads)
  target_link_libraries     (${PROJECT_NAME} INTERFACE Threads::Threads)
endif()

# =============================================================================
# Install
# =============================================================================

install(TARGETS ${PROJECT_NAME})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# =============================================================================
# Examples
# =============================================================================

if(PROMETHEUS_BUILD_EXAMPLES)
  add_subdirectory("examples")
endif()
