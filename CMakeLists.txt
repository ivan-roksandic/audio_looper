cmake_minimum_required(VERSION 3.22)

project("app")

set(CMAKE_CXX_STANDARD 20)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

add_subdirectory(deps/SDL)

find_package(OpenGL REQUIRED)

set(IMGUI_SOURCES
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_demo.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/imgui_widgets.cpp
    deps/imgui/imgui.cpp
    deps/imgui/misc/cpp/imgui_stdlib.cpp
    deps/imgui/backends/imgui_impl_sdl3.cpp
    deps/imgui/backends/imgui_impl_opengl3.cpp

    deps/implot/implot_demo.cpp
    deps/implot/implot_items.cpp
    deps/implot/implot.cpp

    deps/implot3d/implot3d_demo.cpp
    deps/implot3d/implot3d_items.cpp
    deps/implot3d/implot3d_meshes.cpp
    deps/implot3d/implot3d.cpp
)


file(GLOB_RECURSE APP_SOURCES CMAKE_CONFIGURE_DEPENDS src/*.cpp)
add_executable(app ${IMGUI_SOURCES} ${APP_SOURCES})
target_link_libraries(app SDL3::SDL3)
if(EMSCRIPTEN)
target_link_options(app PUBLIC --embed-file=../assets/ --shell-file=../src/emscripten/shell_minimal.html -sALLOW_MEMORY_GROWTH=1 -SFULL_ES3 -sMAX_WEBGL_VERSION=2)
endif()
target_include_directories(app PUBLIC
    include/
    src/
    deps/imgui/ 
    deps/imgui/backends/
    deps/imgui/misc/cpp/
    deps/implot/
    deps/implot3d/
)