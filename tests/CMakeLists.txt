file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/[0-9][0-9]-*.c)

foreach(test_source ${TEST_SOURCES})
  get_filename_component(test_name ${test_source} NAME_WE)
  add_executable(${test_name} ${test_source})
  target_link_libraries(${test_name} cyberiadaml)
  add_test(NAME ${test_name} COMMAND ${test_name}
           WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

file(GLOB TEST_FIXTURES ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/*.graphml)
file(COPY ${TEST_FIXTURES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/fixtures)

file(GLOB TEST_GOLDEN ${CMAKE_CURRENT_SOURCE_DIR}/golden/*)
file(COPY ${TEST_GOLDEN} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/golden)

file(GLOB TEST_SAMPLES ${CMAKE_SOURCE_DIR}/graph-samples/*.graphml)
file(COPY ${TEST_SAMPLES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/samples)
