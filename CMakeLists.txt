cmake_minimum_required(VERSION 3.28)

project(CMakeServer)



set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# FetchContent 모듈 포함

add_subdirectory(NetLib)
add_subdirectory(src)

