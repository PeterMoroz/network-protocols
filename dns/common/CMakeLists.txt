cmake_minimum_required(VERSION 3.0)
project(common)

file(GLOB ${PROJECT_NAME}_SRC "src/*.cpp")
add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRC})
