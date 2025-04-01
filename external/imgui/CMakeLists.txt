cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)

add_library(
  imgui OBJECT
  imgui/imgui.cpp
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
  imgui/backends/imgui_impl_glfw.cpp
  imgui/backends/imgui_impl_opengl3.cpp)

target_include_directories(imgui PUBLIC imgui)

if (NOT EMSCRIPTEN)
  find_package(OpenGL REQUIRED)
  find_package(glfw3 REQUIRED)
endif()

target_include_directories(imgui PUBLIC ${glfw3_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
target_link_libraries(imgui PUBLIC glfw)
