# runs cybparser with ARGS ('|'-separated), checks its exit code against
# EXPECTED and its output against the GOOD file when one is given
string(REPLACE "|" ";" args "${ARGS}")
execute_process(COMMAND ${PARSER} ${args}
                OUTPUT_VARIABLE output
                RESULT_VARIABLE result)
if(NOT result EQUAL EXPECTED)
  message(FATAL_ERROR "cybparser exited with ${result}, ${EXPECTED} expected:\n${output}")
endif()
if(GOOD)
  file(READ ${GOOD} good)
  if(NOT output STREQUAL good)
    message(FATAL_ERROR "cybparser output differs from ${GOOD}:\n${output}")
  endif()
endif()
