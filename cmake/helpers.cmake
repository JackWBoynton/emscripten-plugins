
macro(detect_plugins)
  file(GLOB PLUGINS_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*")

  foreach(PLUGIN_DIR ${PLUGINS_DIRS})
    if(EXISTS "${PLUGIN_DIR}/CMakeLists.txt")
      get_filename_component(PLUGIN_NAME ${PLUGIN_DIR} NAME)
      list(APPEND PLUGINS ${PLUGIN_NAME})
    endif()
  endforeach()

  foreach(PLUGIN_NAME ${PLUGINS})
    message(STATUS "Enabled plugin '${PLUGIN_NAME}'")
  endforeach()
endmacro()


function(detect_and_add_plugins)
  detect_plugins()
  file(MAKE_DIRECTORY "plugins/")

  foreach(plugin IN LISTS PLUGINS)
    message(STATUS "plugin ${plugin}")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/plugins/${plugin}" "${CMAKE_BINARY_DIR}/plugins/${plugin}")
    if(TARGET ${plugin})
      set_target_properties(${plugin} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
      set_target_properties(${plugin} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")

      if(APPLE)
      else()
        if(WIN32)
          get_target_property(target_type ${plugin} TYPE)
          if(target_type STREQUAL "SHARED_LIBRARY")
            install(TARGETS ${plugin} RUNTIME DESTINATION ${PLUGINS_INSTALL_LOCATION})
          else()
            install(TARGETS ${plugin} LIBRARY DESTINATION ${PLUGINS_INSTALL_LOCATION})
          endif()
        else()
          install(TARGETS ${plugin} LIBRARY DESTINATION ${PLUGINS_INSTALL_LOCATION})
        endif()

      endif()

    else()
      message(WARNING "Plugin ${plugin} target not found")
    endif()
  endforeach()
endfunction()
