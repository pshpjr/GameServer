cmake_minimum_required(VERSION 3.28)

option(NETLIB_ENABLE_TRACY "Enable Tracy profiler integration" OFF)
if(NETLIB_ENABLE_TRACY)
    list(APPEND VCPKG_MANIFEST_FEATURES tracy)
endif()

project(CMakeServer)



set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 윈도우 환경에서 빌드 시 출력에 로그가 과하게 나오는 현상 방지용
# vs 설치 시 언어팩 영어도 받아야 함. 
if(MSVC AND CMAKE_GENERATOR MATCHES "Ninja")
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
    set(CMAKE_CL_SHOWINCLUDES_PREFIX "Note: including file: ")
endif()
# FetchContent 모듈 포함

add_subdirectory(NetLib)
add_subdirectory(GameServer)
add_subdirectory(Dummy)
add_subdirectory(NetLibTest_Echo)

include(cmake/BuildVersion.cmake)
target_build_version(CMakeServer SUBMODULES NetLib src/Common)
target_build_version(Dummy SUBMODULES NetLib src/Common Dummy)
