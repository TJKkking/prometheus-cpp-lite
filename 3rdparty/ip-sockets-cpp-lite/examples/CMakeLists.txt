
if(NOT WIN32)
  find_package(Threads)
  target_link_libraries(${PROJECT_NAME} INTERFACE ${CMAKE_THREAD_LIBS_INIT})
endif()

function(add_example NAME_EXAMPLE REQUIRED_TARGETS)
  string(CONCAT SUBPROJECT_NAME "ipsockets_" ${NAME_EXAMPLE})
  message(STATUS "  Adding ${PROJECT_NAME}::${NAME_EXAMPLE} example..")

  add_executable        (${SUBPROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/${NAME_EXAMPLE}.cpp)
  target_link_libraries (${SUBPROJECT_NAME} ${REQUIRED_TARGETS})
  set_target_properties (${SUBPROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/examples)
endfunction()

add_example(ip_address    ip-sockets-cpp-lite)
add_example(udp_socket    ip-sockets-cpp-lite)
add_example(tcp_socket    ip-sockets-cpp-lite)
add_example(resolve_host  ip-sockets-cpp-lite)
add_example(http_server   ip-sockets-cpp-lite)

