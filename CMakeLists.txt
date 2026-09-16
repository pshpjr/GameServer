cmake_minimum_required(VERSION 3.28)

option(NETLIB_ENABLE_TRACY "Enable Tracy profiler integration" OFF)
if(NETLIB_ENABLE_TRACY)
    list(APPEND VCPKG_MANIFEST_FEATURES tracy)
endif()

project(CMakeServer)



set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# FetchContent 모듈 포함

add_subdirectory(NetLib)
add_subdirectory(GameServer)
add_subdirectory(Dummy)
add_subdirectory(NetLibTest_Echo)

include(cmake/BuildVersion.cmake)
target_build_version(CMakeServer SUBMODULES NetLib src/Common)
target_build_version(Dummy SUBMODULES NetLib src/Common Dummy)
