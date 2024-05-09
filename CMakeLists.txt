cmake_minimum_required(VERSION 3.12)

project(cyberiadaml VERSION 1.0)

find_package(LibXml2 REQUIRED)
if(!LibXml2_FOUND)
	message(FATAL_ERROR "Cannot find libxml2 library")
endif()

add_library(cyberiadaml SHARED cyberiadaml.c utf8enc.c cyb_types.c cyb_string.c)
target_include_directories(cyberiadaml PUBLIC
				       "${CMAKE_CURRENT_SOURCE_DIR}"
				       "${LIBXML2_INCLUDE_DIR}")
target_link_directories(cyberiadaml PUBLIC "${LIBXML2_LIBRARY}")
target_link_libraries(cyberiadaml PUBLIC
				  "${LIBXML2_LIBRARIES}"
				  m)

add_executable(cyberiada_test test.c)
target_include_directories(cyberiada_test PUBLIC "${PROJECT_BINARY_DIR}")
target_link_directories(cyberiada_test PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(cyberiada_test PUBLIC cyberiadaml)

install(TARGETS cyberiadaml DESTINATION lib)
install(FILES cyberiadaml.h ${CMAKE_CURRENT_SOURCE_DIR}/cyberiadaml.h
        DESTINATION include/cyberiadaml)
