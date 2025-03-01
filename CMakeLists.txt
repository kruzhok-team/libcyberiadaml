cmake_minimum_required(VERSION 3.12)

project(cyberiadaml VERSION 1.0 LANGUAGES C)

find_package(LibXml2 REQUIRED)
if(!LibXml2_FOUND)
	message(FATAL_ERROR "Cannot find libxml2 library")
endif()

find_package(HTGeom REQUIRED)
if(!HTGeom_FOUND)
  message(FATAL_ERROR "Cannot find libhtgeom library")
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D__DEBUG__")

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  add_compile_options(-Wall)
  add_compile_options(-Wextra) 
  add_compile_options(-Wpedantic)
endif()

add_library(cyberiadaml SHARED
			cyb_actions.c
			cyb_error.h
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
				  "${HTGeom_LIBRARIES}")

add_subdirectory(parser)

install(TARGETS cyberiadaml DESTINATION lib EXPORT cyberiadaml)
install(FILES cyberiadaml.h ${CMAKE_CURRENT_SOURCE_DIR}/cyberiadaml.h
        DESTINATION include/cyberiada)
install(EXPORT cyberiadaml DESTINATION lib/cmake)

configure_file(
   ${PROJECT_SOURCE_DIR}/cyberiadaml-config.cmake.in
   ${PROJECT_BINARY_DIR}/cyberiadaml-config.cmake @ONLY)

install(FILES ${PROJECT_BINARY_DIR}/cyberiadaml-config.cmake
	DESTINATION lib/cmake)
