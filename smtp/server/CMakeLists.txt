cmake_minimum_required(VERSION 3.0)
project(smtp_server)

file(GLOB ${PROJECT_NAME}_SRC "*.cpp")
add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SRC})
# target_link_libraries(${PROJECT_NAME} "pthread" "stdc++fs" "common")
target_link_libraries(${PROJECT_NAME} "pthread" "stdc++fs")