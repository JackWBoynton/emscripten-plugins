cmake_minimum_required(VERSION 3.0)
project(WebPluginExample)

set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set(PLUGIN_SYSTEM_BASE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/plugin.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/helpers.cmake")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(lib INTERFACE)
target_include_directories(lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

detect_and_add_plugins()

#
# Host (executable)
#
add_executable(host main.cpp)

target_link_libraries(host PRIVATE lib)

if (EMSCRIPTEN)
  # For Emscripten main module
  set_target_properties(host PROPERTIES
        OUTPUT_NAME "host"
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/web"
    )
  set_target_properties(host PROPERTIES
      LINK_FLAGS "-s MAIN_MODULE=1 -s EXPORT_ALL=1 -sFULL_ES3=1 -O3"
    )
else()
  # Native: put host in /native directory
  set_target_properties(host PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/native"
    )
  # Link dl on Linux or ignore if not needed on Mac/Windows
  target_link_libraries(host PRIVATE dl)
endif()
