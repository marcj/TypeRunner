cmake_minimum_required(VERSION 3.22)
project(typescript)

#add_definitions(-DTRACY_ENABLE)

#set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3 -ffast-math -fsanitize=leak")
#set(CMAKE_CXX_FLAGS "-Wall -Wextra -O2")

add_subdirectory(tests)

add_library(typescript utf.h utf.cpp core.h core.cpp utilities.h utilities.cpp node_test.h node_test.cpp
        syntax_cursor.h syntax_cursor.cpp parser2.h parser2.cpp types.h types.cpp path.h path.cpp
        factory.h factory.cpp parenthesizer.h parenthesizer.cpp scanner.h scanner.cpp
        checker/vm.h checker/vm.cpp checker/instructions.h checker/compiler.h checker/types.h checker/utils.h checker/checks.h checker/debug.h)
#        ${CMAKE_CURRENT_SOURCE_DIR}/../libs/tracy/TracyClient.cpp

target_link_libraries(typescript fmt)

add_subdirectory(gui)