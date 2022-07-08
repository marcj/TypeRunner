cmake_minimum_required(VERSION 3.22)
project(typescript)

set(CMAKE_CXX_STANDARD 23)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3 -ffast-math")
endif()

include_directories(libs/tracy/)
include_directories(libs/fmt/include/)

list(APPEND CMAKE_MODULE_PATH libs)

#add_definitions(-DTRACY_ENABLE)
#include_directories(libs/tracy/)

add_subdirectory(libs/doctest)
add_subdirectory(libs/tracy)
add_subdirectory(libs/fmt)

set(ASMJIT_STATIC TRUE)
add_subdirectory(libs/asmjit)

# enable for profiling
#add_definitions(-DTRACY_ENABLE)
#link_libraries(Tracy::TracyClient)

include_directories(libs/asmjit/src)
include_directories(libs/magic_enum)

add_subdirectory(src)

add_executable(typescript_main main.cpp)

target_link_libraries(
        typescript_main
        typescript
)