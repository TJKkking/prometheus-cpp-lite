
cmake_minimum_required(VERSION 3.12)
project(ip-sockets-cpp-lite)

# =============================================================================
# Options
# =============================================================================

option(IP_SOCKETS_CPP_LITE_BUILD_EXAMPLES "Build with examples" OFF)

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

# include all header files from current directory
#file(GLOB IP_SOCKETS_CPP_LITE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
# manual include header files
file(GLOB IP_SOCKETS_CPP_LITE_HEADERS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/ip_address.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/udp_socket.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/tcp_socket.h"
)

# =============================================================================
# Main target
# =============================================================================

# it is header only target
add_library               (${PROJECT_NAME}     INTERFACE)
target_sources            (${PROJECT_NAME}     INTERFACE ${IP_SOCKETS_CPP_LITE_HEADERS})
target_include_directories(${PROJECT_NAME}     INTERFACE $<BUILD_INTERFACE:  ${CMAKE_CURRENT_SOURCE_DIR}/include>
                                                         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>  )
# it was need to show header files of INTERFACE target in IDEs like Visual Studio 2015 with CMake < 3.0
#add_custom_target        (${PROJECT_NAME}-ide SOURCES   ${IP_SOCKETS_CPP_LITE_HEADERS})

# =============================================================================
# Install
# =============================================================================

install(TARGETS   ${PROJECT_NAME})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# =============================================================================
# Examples
# =============================================================================

# include examples if the option is enabled
if(IP_SOCKETS_CPP_LITE_BUILD_EXAMPLES)
  add_subdirectory("./examples")
endif()
