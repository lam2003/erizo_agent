cmake_minimum_required(VERSION 2.6)

project (ERIZO_AGENT_CPP_ALL)

set(LIBDEPS_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/libdeps/include")
set(LIBDEPS_LIBARAYS "${CMAKE_CURRENT_SOURCE_DIR}/libdeps/lib")

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}")

###########################################
#set runtime path
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${LIBDEPS_LIBARAYS}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
###########################################
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/erizo_agent_cpp")