cmake_minimum_required(VERSION 3.0)
project(dns)

include_directories("${CMAKE_SOURCE_DIR}/../3rdparty/asio/1.12.2/include")
include_directories("common/include")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(common)
add_subdirectory(server)
