cmake_minimum_required(VERSION 3.26)
project(untitled)

set(CMAKE_CXX_STANDARD 26)

add_executable(untitled main.cpp)

target_link_libraries(untitled ws2_32)