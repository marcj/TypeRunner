project(typescript)

add_executable(typescript_debugger
        debugger_main.cpp
        app.h
        ../../libs/imgui/imgui.cpp
        ../../libs/imgui/imgui_demo.cpp
        ../../libs/imgui/imgui_draw.cpp
        ../../libs/imgui/imgui_widgets.cpp
        ../../libs/imgui/imgui_tables.cpp
        ../../libs/imgui/misc/cpp/imgui_stdlib.cpp
        ../../libs/imgui/backends/imgui_impl_sdl.cpp
        ../../libs/imgui/backends/imgui_impl_opengl3.cpp
        ../../libs/imgui-texteditor-fork/TextEditor.cpp
)

target_include_directories(
        typescript_debugger
        PUBLIC ../../libs/imgui
        PUBLIC ../../libs/imgui-texteditor-fork
        PUBLIC /usr/local/include/
        PUBLIC ../../libs/imgui/backends
        PUBLIC ../../libs/imgui/examples/libs/gl3w/
)

#if (TRACY_ENABLE)
#    target_sources(
#            typescript_debugger
#            PUBLIC ../../libs/imgui/examples/libs/gl3w/GL/gl3w.c
#    )
#endif()

target_link_libraries(typescript_debugger typescript)

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)

target_include_directories(
        typescript_debugger
        PUBLIC ${SDL2_INCLUDE_DIRS}
)

if (APPLE)
    target_link_libraries(typescript_debugger "-framework CoreFoundation")
endif ()

target_link_libraries(
        typescript_debugger
        ${SDL2_LIBRARIES}
        ${OPENGL_LIBRARIES}
)