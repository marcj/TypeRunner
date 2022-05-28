cmake_minimum_required(VERSION 3.22)
project(typescript)

add_definitions(-DTRACY_ENABLE)

add_subdirectory(tests)

#add_library(typescript parser.cpp scanner.cpp utf.cpp syntax_cursor.cpp syntax_cursor.h ast.h parser2.h)
add_library(typescript scanner.cpp utf.cpp syntax_cursor.cpp syntax_cursor.h ast.h parser2.h types.cpp node_test.h path.h node_factory.h)
#add_library(typescript scanner.cpp utf.cpp syntax_cursor.cpp syntax_cursor.h ast.h)