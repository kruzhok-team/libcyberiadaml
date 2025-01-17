cmake_minimum_required(VERSION 3.12)

project(cybparser VERSION 1.0)
add_executable(cybparser parser.c)
target_include_directories(cybparser PUBLIC
			   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			   $<INSTALL_INTERFACE:include/cyberiada>)
target_link_directories(cybparser PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(cybparser PUBLIC cyberiadaml)
