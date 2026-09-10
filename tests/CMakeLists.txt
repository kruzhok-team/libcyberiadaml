file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/[0-9][0-9]-*.c)

if(MSVC)
  # the test uses pthreads, unavailable in the MSVC runtime
  list(FILTER TEST_SOURCES EXCLUDE REGEX "18-key-remap")
endif()

foreach(test_source ${TEST_SOURCES})
  get_filename_component(test_name ${test_source} NAME_WE)
  add_executable(${test_name} ${test_source})
  target_link_libraries(${test_name} cyberiadaml)
  add_test(NAME ${test_name} COMMAND ${test_name}
           WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
    # the dependency DLLs must sit next to the test executable
    add_custom_command(TARGET ${test_name}
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
              "$<TARGET_RUNTIME_DLLS:${test_name}>" "$<TARGET_FILE_DIR:${test_name}>"
      COMMAND_EXPAND_LISTS
      )
  endif()
endforeach()

if(TARGET 18-key-remap)
  find_package(Threads REQUIRED)
  target_link_libraries(18-key-remap Threads::Threads)
endif()

file(GLOB TEST_DIAGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/diagrams/*.graphml)
file(COPY ${TEST_DIAGRAMS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/diagrams)

file(GLOB TEST_GOOD ${CMAKE_CURRENT_SOURCE_DIR}/good/*)
file(COPY ${TEST_GOOD} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/good)

file(GLOB TEST_SAMPLES ${CMAKE_SOURCE_DIR}/graph-samples/*.graphml)
file(COPY ${TEST_SAMPLES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/samples)
