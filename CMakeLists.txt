cmake_minimum_required(VERSION 3.22)
project(typescript)

set(CMAKE_CXX_STANDARD 20)

# enable for profiling
#add_definitions(-DTRACY_ENABLE)

include_directories(libs/tracy/)

list(APPEND CMAKE_MODULE_PATH libs/)

#add_definitions(-DTRACY_ENABLE)
#include_directories(libs/tracy/)

add_subdirectory(libs/tracy)

link_libraries(Tracy::TracyClient)
add_subdirectory(src)

include_directories(libs)

add_executable(typescript_main main.cpp)

target_link_libraries(
        typescript_main
        typescript
)