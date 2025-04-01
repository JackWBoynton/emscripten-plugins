cmake_minimum_required(VERSION 3.20)
project(WebPluginExample)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set(PLUGIN_SYSTEM_BASE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(PLUGINS_INSTALL_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/plugins)

add_subdirectory(external/imgui)

set(LIB_SOURCES 
    src/plugin_manager.cpp
    src/app_host.cpp
)

if (NOT EMSCRIPTEN)
  find_package(CURL REQUIRED)
  find_package(OpenGL REQUIRED)
  find_package(glfw3 REQUIRED)
endif()

add_subdirectory(external/json)

add_library(lib STATIC ${LIB_SOURCES})
target_include_directories(lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)
target_link_libraries(lib PUBLIC imgui ${glfw3_LIBRARIES} ${CURL_LIBRARIES} ${OPENGL_LIBRARIES} nlohmann_json::nlohmann_json)
get_target_property(imgui_incl imgui INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(nlohmann_json_incl nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(lib PUBLIC ${imgui_incl} ${nlohmann_json_incl})

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/plugin.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/helpers.cmake")

#
# Host (executable)
#
add_executable(host main.cpp)


if (EMSCRIPTEN)
  set_target_properties(host PROPERTIES
        OUTPUT_NAME "host"
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/web"
    )
  set_target_properties(host PROPERTIES
      LINK_FLAGS " -s EXPORT_ALL=1 -s FETCH=1 -O0 -gsource-map -s USE_GLFW=3 -sFETCH=1 -s FULL_ES3=1 -g4 -sASSERTIONS=1 -s MAIN_MODULE=1 -s USE_FREETYPE=1 -sERROR_ON_UNDEFINED_SYMBOLS=0 -lidbfs.js -sFORCE_FILESYSTEM=1"
  )

else()
  set_target_properties(host PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/native"
    )
  target_link_libraries(host PRIVATE dl)
endif()

detect_and_add_plugins()

target_link_libraries(host PUBLIC lib)
