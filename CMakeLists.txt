cmake_minimum_required(VERSION 3.12)

project(cyberiadaml VERSION 1.0)

find_package(LibXml2 REQUIRED)
if(!LibXml2_FOUND)
	message(FATAL_ERROR "Cannot find libxml2 library")
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D__DEBUG__")

add_library(cyberiadaml SHARED cyberiadaml.c utf8enc.c cyb_types.c cyb_string.c geometry.c)
target_include_directories(cyberiadaml PUBLIC
				       $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			               $<INSTALL_INTERFACE:include/cyberiada>
				       "${LIBXML2_INCLUDE_DIR}")
target_link_directories(cyberiadaml PUBLIC "${LIBXML2_LIBRARY}")
target_link_libraries(cyberiadaml PUBLIC
				  "${LIBXML2_LIBRARIES}"
				  m)

add_executable(cyberiada_test test.c)
target_include_directories(cyberiada_test PUBLIC
			   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			   $<INSTALL_INTERFACE:include/cyberiada>)
target_link_directories(cyberiada_test PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(cyberiada_test PUBLIC cyberiadaml)

install(TARGETS cyberiadaml DESTINATION lib EXPORT cyberiadaml)
install(FILES cyberiadaml.h ${CMAKE_CURRENT_SOURCE_DIR}/cyberiadaml.h
        DESTINATION include/cyberiada)
install(EXPORT cyberiadaml DESTINATION lib/cmake)

configure_file(
   ${PROJECT_SOURCE_DIR}/cyberiadaml-config.cmake.in
   ${PROJECT_BINARY_DIR}/cyberiadaml-config.cmake @ONLY)

install(FILES ${PROJECT_BINARY_DIR}/cyberiadaml-config.cmake
	DESTINATION lib/cmake)
