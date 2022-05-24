cmake_minimum_required(VERSION 3.22)
project(typescript)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -O2")
list(APPEND CMAKE_MODULE_PATH libs/)

#add_definitions(-DTRACY_ENABLE)
#include_directories(libs/tracy/)

add_subdirectory(src)

include_directories(libs)

add_executable(typescript_main main.cpp)

target_link_libraries(
        typescript_main
        typescript
)