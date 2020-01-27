cmake_minimum_required(VERSION 3.0)
project(dns-client)

# include_directories("${CMAKE_SOURCE_DIR}/../../3rdparty/asio/1.12.2/include")

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_BUILD_TYPE Debug)

file(GLOB ${PROJECT_NAME}_SRC "*.cpp")
add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRC})
target_link_libraries(${PROJECT_NAME} "pthread" "common")
