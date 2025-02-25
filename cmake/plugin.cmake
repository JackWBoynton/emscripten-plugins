
function(add_plugin parent libs ${ARGN})

  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(options LIBRARY_PLUGIN)
  set(oneValueArgs NAME)
  set(multiValueArgs SOURCES INCLUDES SYSTEM_INCLUDES LIBRARIES TESTS DEPENDS)
  cmake_parse_arguments(PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  message(STATUS "compiling plugin ${PLUGIN_NAME}")

  if(NOT TARGET ${parent})
    message(FATAL_ERROR "Target ${parent} does not exist")
  endif()

  if(DEFINED PLUGIN_NAME)
    message(STATUS "Compiling plugin ${PLUGIN_NAME}")
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
      set(PLUGIN_LIBRARY_TYPE SHARED) # todo: benchmark vs MODULE
    endif()
  endif()

  message(STATUS "Adding plugin ${PLUGIN_NAME} to ${parent}")
  project(${PLUGIN_NAME})

  if (EMSCRIPTEN)
    add_executable(${PLUGIN_NAME} ${PLUGIN_SOURCES})
  else()
    add_library(${PLUGIN_NAME} ${PLUGIN_LIBRARY_TYPE} ${PLUGIN_SOURCES})
  endif()

  # Add include directories and link libraries
  target_include_directories(${PLUGIN_NAME} PUBLIC ${PLUGIN_INCLUDES})
  target_include_directories(${PLUGIN_NAME} SYSTEM PUBLIC ${PLUGIN_SYSTEM_INCLUDES})
  target_link_libraries(${PLUGIN_NAME} PUBLIC ${PLUGIN_LIBRARIES})
  target_link_libraries(${PLUGIN_NAME} PRIVATE ${parent})

  target_compile_definitions(${PLUGIN_NAME} PRIVATE PLUGIN_NAME=${PLUGIN_NAME})

  # setup_compiler_flags(${PLUGIN_NAME})

  # Configure build properties
  set_target_properties(
    ${PLUGIN_NAME}
    PROPERTIES CXX_STANDARD 23
               PREFIX ""
               SUFFIX ${PLUGIN_SUFFIX})


  if (EMSCRIPTEN)
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        SUFFIX "${PLUGIN_SUFFIX}.wasm"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        LINK_FLAGS "-sSIDE_MODULE=1 -O0 -sEXPORT_ALL=1 -s STANDALONE_WASM --no-entry"
    )
  endif()

  if(EXTERNAL_PLUGIN_BUILD)
    install(TARGETS ${PLUGIN_NAME} DESTINATION ".")
  endif()

  # Fix rpath
  if(APPLE)
    set_target_properties(${PLUGIN_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../Frameworks;@executable_path/plugins")
  elseif(UNIX)
    set(PLUGIN_RPATH "")
    list(APPEND PLUGIN_RPATH "$ORIGIN")

    set_target_properties(${PLUGIN_NAME} PROPERTIES INSTALL_RPATH_USE_ORIGIN ON INSTALL_RPATH "${PLUGIN_RPATH}
      ")
  endif()


endfunction()

