cmake_minimum_required(VERSION 3.22)
project(typescript)

set(CMAKE_CXX_STANDARD 23)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3 -ffast-math")
endif()

include_directories(libs/tracy/)
include_directories(libs/fmt/include/)
include_directories(libs)

list(APPEND CMAKE_MODULE_PATH libs/)

#add_definitions(-DTRACY_ENABLE)
#include_directories(libs/tracy/)

add_subdirectory(libs/tracy)
add_subdirectory(libs/fmt)

# enable for profiling
#add_definitions(-DTRACY_ENABLE)
#link_libraries(Tracy::TracyClient)

add_subdirectory(src)

include_directories(libs)

add_executable(typescript_main main.cpp)

target_link_libraries(
        typescript_main
        typescript
)