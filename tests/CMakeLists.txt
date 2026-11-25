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
    if(HTGeom_RUNTIME_LIBRARY)
      add_custom_command(TARGET ${test_name}
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${HTGeom_RUNTIME_LIBRARY}" "$<TARGET_FILE_DIR:${test_name}>"
        )
    endif()
  endif()
endforeach()

if(TARGET 18-key-remap)
  find_package(Threads REQUIRED)
  target_link_libraries(18-key-remap Threads::Threads)
endif()

file(GLOB TEST_DIAGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/diagrams/*.graphml)
file(COPY ${TEST_DIAGRAMS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/diagrams)

file(GLOB TEST_FA_DIAGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/diagrams/fa/*.graphml)
file(COPY ${TEST_FA_DIAGRAMS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/diagrams/fa)

file(GLOB TEST_GOOD ${CMAKE_CURRENT_SOURCE_DIR}/good/*)
file(COPY ${TEST_GOOD} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/good)

file(GLOB TEST_SAMPLES ${CMAKE_SOURCE_DIR}/graph-samples/*.graphml)
file(COPY ${TEST_SAMPLES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/samples)

# the cybparser comparisons; a cmake script runs cybparser, without the emulator of the
# Windows cross build
if(TARGET cybparser AND NOT WIN32)
  foreach(parser_case
      "parser-diff-same|0|diff|-g|diagrams/diff-light.graphml|-o|diagrams/diff-light.graphml|"
      "parser-diff-light|1|diff|-g|diagrams/diff-light.graphml|-o|diagrams/diff-light-edited.graphml|good/parser-diff-light.txt"
      "parser-isomorph|0|isomorph|-g|diagrams/minimal.graphml|-o|diagrams/minimal.graphml|"
      "parser-listing-diff-light|0|listing|diagrams/diff-light.graphml|good/parser-listing-diff-light.txt"
      "parser-listing-completeness|0|listing|diagrams/completeness.graphml|good/parser-listing-completeness.txt"
      "parser-listing-event-propagation|0|listing|diagrams/event-propagation.graphml|good/parser-listing-event-propagation.txt"
      "parser-listing-comment-sizes|0|listing|diagrams/comment-sizes.graphml|good/parser-listing-comment-sizes.txt"
      "parser-listing-two-machines|0|listing|diagrams/two-machines.graphml|good/parser-listing-two-machines.txt"
      "parser-listing-two-regions|0|listing|diagrams/two-regions.graphml|good/parser-listing-two-regions.txt"
      "parser-listing-format|0|listing|-g|diagrams/diff-light.graphml|-f|cyberiada|good/parser-listing-diff-light.txt"
      "parser-listing-empty|0|listing|diagrams/empty.graphml|diagrams/empty.graphml"
      "parser-listing-not-read|0|listing|diagrams/bad.graphml|diagrams/bad.graphml"
      "parser-listing-missing|2|listing|diagrams/missing.graphml|")
    # the fields: the name, the exit code, the arguments, the good file (may be empty)
    string(REPLACE "|" ";" parser_fields "${parser_case}")
    list(LENGTH parser_fields parser_size)
    math(EXPR parser_last "${parser_size} - 1")
    math(EXPR parser_args_size "${parser_size} - 3")
    list(GET parser_fields 0 parser_name)
    list(GET parser_fields 1 parser_expected)
    list(SUBLIST parser_fields 2 ${parser_args_size} parser_args)
    list(GET parser_fields ${parser_last} parser_good)
    string(REPLACE ";" "|" parser_args "${parser_args}")
    add_test(NAME ${parser_name}
             COMMAND ${CMAKE_COMMAND} -DPARSER=$<TARGET_FILE:cybparser> -DARGS=${parser_args}
                     -DEXPECTED=${parser_expected} -DGOOD=${parser_good}
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/run-parser.cmake
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endforeach()

  find_program(GIT_EXECUTABLE git)
  if(GIT_EXECUTABLE)
    add_test(NAME parser-listing-git
             COMMAND ${CMAKE_COMMAND} -DPARSER=$<TARGET_FILE:cybparser> -DGIT=${GIT_EXECUTABLE}
                     -DDIAGRAM=${CMAKE_CURRENT_SOURCE_DIR}/diagrams/diff-light.graphml
                     -DEDITED=${CMAKE_CURRENT_SOURCE_DIR}/diagrams/diff-light-edited.graphml
                     -P ${CMAKE_CURRENT_SOURCE_DIR}/run-textconv.cmake
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endif()
endif()

# cybparser listing of a non-ASCII (UTF-8) file name, Windows included: git hands the
# textconv a file of the diagram's own name
if(TARGET cybparser)
  configure_file(diagrams/diff-light.graphml
                 "${CMAKE_CURRENT_BINARY_DIR}/diagrams/светофор.graphml" COPYONLY)
  add_test(NAME parser-listing-utf8
           COMMAND cybparser listing "diagrams/светофор.graphml"
           WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  set_tests_properties(parser-listing-utf8 PROPERTIES
                       PASS_REGULAR_EXPRESSION "\n  transition Red -> Night \\[NIGHT\\]\n"
                       FAIL_REGULAR_EXPRESSION "cannot|not read")
endif()
