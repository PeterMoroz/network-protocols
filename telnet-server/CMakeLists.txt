cmake_minimum_required(VERSION 3.0)
project(telnet-server)

include_directories("${CMAKE_SOURCE_DIR}/../3rdparty/asio/1.12.2/include")

add_executable(telnet-server main.cpp server.cpp session.cpp date_time_utils.cpp)
target_link_libraries(telnet-server "pthread")
target_compile_features(telnet-server PRIVATE cxx_std_17)
