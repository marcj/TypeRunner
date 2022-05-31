cmake_minimum_required(VERSION 3.22)
project(typescript)

add_definitions(-DTRACY_ENABLE)

add_subdirectory(tests)

add_library(typescript utf.h utf.cpp core.h core.cpp utilities.h utilities.cpp node_test.h node_test.cpp
        syntax_cursor.h syntax_cursor.cpp parser2.h parser2.cpp types.h types.cpp path.h path.cpp
        factory.h factory.cpp parenthesizer.h parenthesizer.cpp scanner.h scanner.cpp)
