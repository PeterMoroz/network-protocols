cmake_minimum_required(VERSION 3.0)
project(telnet)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)

include_directories("${CMAKE_SOURCE_DIR}/../3rdparty/asio/1.12.2/include")

add_subdirectory(server)