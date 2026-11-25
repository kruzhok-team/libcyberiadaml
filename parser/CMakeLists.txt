cmake_minimum_required(VERSION 3.12)

project(cybparser VERSION 1.2.0)
add_executable(cybparser parser.c listing.c)
target_include_directories(cybparser PUBLIC
			   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			   $<INSTALL_INTERFACE:include/cyberiada>)
target_link_directories(cybparser PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(cybparser PUBLIC cyberiadaml)
if(WIN32)
  # CommandLineToArgvW: the arguments in UTF-8
  target_link_libraries(cybparser PRIVATE shell32)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
    # the dependency DLLs next to cybparser in the build tree, for the tests
    add_custom_command(TARGET cybparser
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
              "$<TARGET_RUNTIME_DLLS:cybparser>" "$<TARGET_FILE_DIR:cybparser>"
      COMMAND_EXPAND_LISTS
      )
    if(HTGeom_RUNTIME_LIBRARY)
      add_custom_command(TARGET cybparser
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${HTGeom_RUNTIME_LIBRARY}" "$<TARGET_FILE_DIR:cybparser>"
        )
    endif()
  endif()
endif()

install(TARGETS cybparser DESTINATION bin
        COMPONENT parser)
