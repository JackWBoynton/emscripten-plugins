

if(NOT TARGET lib_wrap)
add_library(lib_wrap INTERFACE)
get_target_property(incs lib INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(lib_wrap INTERFACE ${incs})
endif()


function(add_plugin parent ${ARGN})
  set(options LIBRARY_PLUGIN)
  set(oneValueArgs NAME)
  set(multiValueArgs SOURCES INCLUDES SYSTEM_INCLUDES LIBRARIES TESTS DEPENDS)
  cmake_parse_arguments(PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  message(STATUS "compiling plugin ${PLUGIN_NAME}")

  if(NOT TARGET ${parent})
    message(FATAL_ERROR "Target ${parent} does not exist")
  endif()

  if(STATIC_LINK_PLUGINS)
    set(PLUGIN_LIBRARY_TYPE STATIC)

    target_link_libraries(${parent} PRIVATE ${PLUGIN_NAME})

    configure_file(${CMAKE_SOURCE_DIR}/cmake/plugin-static.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/plugin-static.cpp @ONLY)
    target_sources(${parent} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/plugin-static.cpp)
    set(PLUGIN_SUFFIX ".plugin")
  else()
    if(PLUGIN_LIBRARY_PLUGIN)
      set(PLUGIN_LIBRARY_TYPE SHARED)
      set(PLUGIN_SUFFIX ".plugin_lib")
    else()
      set(PLUGIN_SUFFIX ".plugin")
      set(PLUGIN_LIBRARY_TYPE MODULE) # todo: benchmark vs MODULE
    endif()
  endif()

  message(STATUS "Adding plugin ${PLUGIN_NAME} to ${parent}")
  project(${PLUGIN_NAME})
  if (EMSCRIPTEN)
    add_executable(${PLUGIN_NAME} ${PLUGIN_SOURCES})
  else()
    add_library(${PLUGIN_NAME} ${PLUGIN_LIBRARY_TYPE} ${PLUGIN_SOURCES})
  endif()

  # Configure build properties
  if (EMSCRIPTEN)
    set_target_properties(
    ${PLUGIN_NAME}
    PROPERTIES CXX_STANDARD 23
               PREFIX ""
               SUFFIX ${PLUGIN_SUFFIX}.wasm
  )
  target_compile_definitions(${PLUGIN_NAME} PUBLIC DEBUG=1)
  target_link_options(${PLUGIN_NAME} PRIVATE -sSIDE_MODULE=1 -O0 -sWASM=1 -sEXPORT_ALL=1 --no-entry -sASSERTIONS=1 -fwasm-exceptions -v -sERROR_ON_UNDEFINED_SYMBOLS=0 -gsource-map)
  else()
    set_target_properties(
    ${PLUGIN_NAME}
    PROPERTIES CXX_STANDARD 23
               PREFIX ""
               SUFFIX ${PLUGIN_SUFFIX}
  )
  endif()

  # Add include directories and link libraries
  target_include_directories(${PLUGIN_NAME} PUBLIC ${PLUGIN_INCLUDES})
  target_include_directories(${PLUGIN_NAME} SYSTEM PUBLIC ${PLUGIN_SYSTEM_INCLUDES})
  target_link_libraries(${PLUGIN_NAME} PUBLIC ${PLUGIN_LIBRARIES})
  target_link_libraries(${PLUGIN_NAME} PRIVATE lib_wrap)

  target_compile_definitions(${PLUGIN_NAME} PRIVATE PLUGIN_NAME=${PLUGIN_NAME})

  message(STATUS "OUTPUT DIR: ${CMAKE_BINARY_DIR}/plugins")


  if(EXTERNAL_PLUGIN_BUILD)
    install(TARGETS ${PLUGIN_NAME} DESTINATION ".")
  endif()

  # Fix rpath
  if(APPLE AND NOT EMSCRIPTEN)
    set_target_properties(${PLUGIN_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../Frameworks;@executable_path/plugins")
  elseif(UNIX OR EMSCRIPTEN)
    set(PLUGIN_RPATH "")
    list(APPEND PLUGIN_RPATH "$ORIGIN")

    set_target_properties(${PLUGIN_NAME} PROPERTIES INSTALL_RPATH_USE_ORIGIN ON INSTALL_RPATH "${PLUGIN_RPATH}
      ")
  endif()

  # TODO: make this only for certain symbols
  # allow undefined symbols
  if(APPLE AND NOT EMSCRIPTEN)
    set_target_properties(${PLUGIN_NAME} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
  elseif (UNIX OR EMSCRIPTEN)
    # allow plugis to resolve symbols from the executable who loads them
    set_target_properties(${PLUGIN_NAME} PROPERTIES LINK_FLAGS "-rdynamic")
  endif()


endfunction()

