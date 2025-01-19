cmake_minimum_required(VERSION 3.0)
project(WebPluginExample)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set(PLUGIN_SYSTEM_BASE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/plugin.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/helpers.cmake")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(PLUGINS_INSTALL_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/plugins)

add_library(lib INTERFACE)
target_include_directories(lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

detect_and_add_plugins()

#
# Host (executable)
#
add_executable(host main.cpp)


if (EMSCRIPTEN)
  # For Emscripten main module
  set_target_properties(host PROPERTIES
        OUTPUT_NAME "host"
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/web"
    )
  set_target_properties(host PROPERTIES
      LINK_FLAGS " -s EXPORT_ALL=1 -s FETCH=1 -O3 -s USE_GLFW=3 -sFETCH=1 -s FULL_ES3=1 -g4 -sASSERTIONS=2 -s MAIN_MODULE=1 -s USE_FREETYPE=1"
  )
else()

  find_package(curl REQUIRED)
  find_package(OpenGL REQUIRED)
  # Native: put host in /native directory
  set_target_properties(host PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/native"
    )
  # Link dl on Linux or ignore if not needed on Mac/Windows
  target_link_libraries(host PRIVATE dl ${CURL_LIBRARIES} ${OPENGL_LIBRARIES})
endif()

# External dependencies
add_subdirectory(external/imgui)

target_link_libraries(host PRIVATE lib imgui)
