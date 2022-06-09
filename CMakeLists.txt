cmake_minimum_required(VERSION 3.16)
project(LoomisChat VERSION 0.1.0)

enable_language(C)

include_directories(dependencies/json-c/build)
include_directories(dependencies/wsServer/include)

add_executable(chatserver server.c)

find_package(json-c CONFIG)
target_link_libraries(chatserver PRIVATE json-c::json-c)
target_link_libraries(chatserver PRIVATE ${CMAKE_SOURCE_DIR}/dependencies/wsServer/build/libws.a)
