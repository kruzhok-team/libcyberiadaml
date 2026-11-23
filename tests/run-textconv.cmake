# git diff through the textconv driver `cybparser listing`: a repository of the test with
# a git configuration of its own, a diagram committed and then edited; the diff shows the
# elements of the diagram, not its GraphML lines
set(work ${CMAKE_CURRENT_BINARY_DIR}/textconv)
set(repo ${work}/repo)
file(REMOVE_RECURSE ${work})
file(MAKE_DIRECTORY ${repo})
file(WRITE ${work}/gitconfig "")
set(ENV{GIT_CONFIG_GLOBAL} ${work}/gitconfig)
set(ENV{GIT_CONFIG_NOSYSTEM} 1)
set(ENV{HOME} ${work})

function(run_git)
  execute_process(COMMAND ${GIT} ${ARGN}
                  WORKING_DIRECTORY ${repo}
                  RESULT_VARIABLE result
                  OUTPUT_VARIABLE output
                  ERROR_VARIABLE error)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "git ${ARGN} failed (${result}):\n${error}")
  endif()
  set(git_output "${output}" PARENT_SCOPE)
endfunction()

run_git(init -q)
run_git(config user.name "Cyberiada Test")
run_git(config user.email "test@localhost")
run_git(config diff.cyberiada.textconv "${PARSER} listing")
file(WRITE ${repo}/.gitattributes "*.graphml diff=cyberiada\n")
configure_file(${DIAGRAM} ${repo}/light.graphml COPYONLY)
run_git(add -A)
run_git(commit -q -m light)
configure_file(${EDITED} ${repo}/light.graphml COPYONLY)
run_git(diff --no-color)

string(FIND "${git_output}" "\n+  state Blink\n" added)
string(FIND "${git_output}" "<node" xml)
if(added EQUAL -1 OR NOT xml EQUAL -1)
  message(FATAL_ERROR "git diff does not show the listing:\n${git_output}")
endif()
