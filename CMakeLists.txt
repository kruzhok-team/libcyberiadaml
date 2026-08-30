cmake_minimum_required(VERSION 3.12)

project(cyberiadaml VERSION 1.0.6 LANGUAGES C)

find_package(LibXml2 2.9 REQUIRED)
if(NOT LibXml2_FOUND)
	message(FATAL_ERROR "Cannot find libxml2 library")
endif()

find_package(HTGeom REQUIRED)
if(NOT HTGeom_FOUND)
  message(FATAL_ERROR "Cannot find libhtgeom library")
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D__DEBUG__")

add_compile_options(-Wall)
add_compile_options(-Wextra) 
add_compile_options(-Wpedantic)
add_compile_options(-Wformat-security)

add_library(cyberiadaml SHARED
			cyb_actions.c
			cyb_error.h
			cyb_regex.h
			cyb_graph.c		
			cyb_graph_recon.c	
			cyb_node_stack.c
			cyb_meta.c
			cyb_regexps.c
			cyb_string.c
			cyb_structs.c
	 		cyb_types.c
			geometry.c
			isomorph.c
			utf8enc.c
			cyberiadaml.c
			cyberiadaml_iso.h
)

target_include_directories(cyberiadaml PUBLIC
				       $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			               $<INSTALL_INTERFACE:include/cyberiada>
				       "${LIBXML2_INCLUDE_DIR}"
				       "${HTGeom_INCLUDE_DIR}")
target_link_directories(cyberiadaml PUBLIC
				    "${LIBXML2_LIBRARY}"
				    "${HTGeom_LIBRARY}")
target_link_libraries(cyberiadaml PUBLIC
				  "${LIBXML2_LIBRARIES}"
				  "${HTGeom_LIBRARIES}"
				  $<IF:$<PLATFORM_ID:Linux>,,pcre2-posix>)

target_compile_definitions(cyberiadaml PRIVATE
					   $<$<NOT:$<PLATFORM_ID:Linux>>:CYBERIADA_PCRE2_REGEXPS>)

add_subdirectory(parser)

set(MEMORYCHECK_COMMAND_OPTIONS "--tool=memcheck --leak-check=full --error-exitcode=1")
include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

install(TARGETS cyberiadaml DESTINATION lib EXPORT cyberiadaml
        COMPONENT runtime)
install(FILES cyberiadaml.h cyberiadaml_iso.h
        DESTINATION include/cyberiada
        COMPONENT dev)
install(EXPORT cyberiadaml DESTINATION lib/cmake
        COMPONENT dev)

configure_file(
   ${PROJECT_SOURCE_DIR}/cyberiadaml-config.cmake.in
   ${PROJECT_BINARY_DIR}/cyberiadaml-config.cmake @ONLY)

install(FILES ${PROJECT_BINARY_DIR}/cyberiadaml-config.cmake
	DESTINATION lib/cmake
	COMPONENT dev)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/docs/API.md
	      ${CMAKE_CURRENT_SOURCE_DIR}/docs/ARCHITECTURE.md
	DESTINATION share/doc/libcyberiadaml
	COMPONENT dev)

configure_file ("${PROJECT_SOURCE_DIR}/CyberiadaMLPackOptions.cmake.in"
                "${PROJECT_BINARY_DIR}/CyberiadaMLPackOptions.cmake"
                @ONLY)
set (CPACK_PROJECT_CONFIG_FILE
     "${PROJECT_BINARY_DIR}/CyberiadaMLPackOptions.cmake")

include(CPack)
