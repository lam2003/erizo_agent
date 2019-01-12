cmake_minimum_required(VERSION 2.8)

project (ERIZO_AGENT_CPP)

set(ERIZO_AGENT_CPP_VERSION_MAJOR 0)
set(ERIZO_AGENT_CPP_VERSION_MINOR 1)

set(CMAKE_CXX_FLAGS "-g -Wall  -Wno-deprecated-declarations -O3 -std=c++11 ${ERIZO_CMAKE_CXX_FLAGS}")

include_directories("${ERIZO_AGENT_CPP_SOURCE_DIR}" "${LIBDEPS_INCLUDE}")
link_directories("${LIBDEPS_LIBARAYS}")

file(GLOB_RECURSE ERIZO_AGENT_CPP_SOURCES "${ERIZO_AGENT_CPP_SOURCE_DIR}/*.h" "${ERIZO_AGENT_CPP_SOURCE_DIR}/*.c" "${ERIZO_AGENT_CPP_SOURCE_DIR}/*.cpp" "${ERIZO_AGENT_CPP_SOURCE_DIR}/*.cc")

add_executable(erizo_agent_cpp ${ERIZO_AGENT_CPP_SOURCES})

target_link_libraries(erizo_agent_cpp rabbitmq log4cxx pthread jsoncpp erizo boost_system)

install(TARGETS erizo_agent_cpp RUNTIME DESTINATION bin)

