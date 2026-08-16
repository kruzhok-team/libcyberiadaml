file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/[0-9][0-9]-*.c)

foreach(test_source ${TEST_SOURCES})
  get_filename_component(test_name ${test_source} NAME_WE)
  add_executable(${test_name} ${test_source})
  target_link_libraries(${test_name} cyberiadaml)
  add_test(NAME ${test_name} COMMAND ${test_name}
           WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

find_package(Threads REQUIRED)
target_link_libraries(18-key-remap Threads::Threads)

file(GLOB TEST_DIAGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/diagrams/*.graphml)
file(COPY ${TEST_DIAGRAMS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/diagrams)

file(GLOB TEST_GOOD ${CMAKE_CURRENT_SOURCE_DIR}/good/*)
file(COPY ${TEST_GOOD} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/good)

file(GLOB TEST_SAMPLES ${CMAKE_SOURCE_DIR}/graph-samples/*.graphml)
file(COPY ${TEST_SAMPLES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/samples)
