cmake_minimum_required(VERSION 3.0)
project(tftp)

include_directories("${CMAKE_SOURCE_DIR}/../3rdparty/asio/1.12.2/include")

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Debug)
add_executable(tftp tftp-client.cpp)
target_link_libraries(tftp "pthread" "stdc++fs")
add_executable(tftp-server tftp-server.cpp)
target_link_libraries(tftp-server "pthread" "stdc++fs")