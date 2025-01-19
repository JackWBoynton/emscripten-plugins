
set(CMAKE_MODULE_PATH "${PLUGIN_SYSTEM_BASE_FOLDER}/cmake")
include("${PLUGIN_SYSTEM_BASE_FOLDER}/cmake/plugin.cmake")

add_plugin(
  lib
  ""
  NAME
  plugin_b
  SOURCES
  main.cpp
)
