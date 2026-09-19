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

if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
  # htgeom is linked through its import library, which does not tell where the
  # DLL is, so the tests cannot get it from $<TARGET_RUNTIME_DLLS>. Under a
  # cross-build the tests run via the emulator with the prefix on its DLL path
  # (WINEPATH), so no copy is needed
  get_filename_component(HTGeom_LIBRARY_DIR "${HTGeom_LIBRARY}" DIRECTORY)
  find_file(HTGeom_RUNTIME_LIBRARY NAMES htgeom.dll
            HINTS "${HTGeom_LIBRARY_DIR}" "${HTGeom_LIBRARY_DIR}/.." "${HTGeom_DIR}"
            PATH_SUFFIXES bin lib)
  if(NOT HTGeom_RUNTIME_LIBRARY)
    message(WARNING "Cannot find htgeom.dll; copy it next to the executables manually")
  endif()
endif()

add_compile_definitions($<$<CONFIG:Debug>:__DEBUG__>)

if(MSVC)
  add_compile_options(/W4)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall)
  add_compile_options(-Wextra) 
  add_compile_options(-Wpedantic)
  add_compile_options(-Wformat-security)
  if(WIN32)
    # MinGW's -Wformat uses the ms_printf archetype and rejects %zu even with
    # __USE_MINGW_ANSI_STDIO (which selects the C99 printf at runtime); the
    # warning is spurious there, so silence the format checks on this target
    add_compile_options(-Wno-format)
  endif()
endif()

if(WIN32)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  # use MinGW's C99-conformant printf so %zu works (msvcrt's does not support it)
  add_compile_definitions(__USE_MINGW_ANSI_STDIO=1)
endif()

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

# the public header includes <cyberiada/htgeom.h>, so the parent of the
# directory holding htgeom.h is needed, not the directory itself
get_filename_component(HTGeom_INCLUDE_ROOT "${HTGeom_INCLUDE_DIR}" DIRECTORY)

# the htgeom include dirs come from the find-module as absolute build-prefix
# paths, so they are build-only: baking them into the export makes the installed
# config non-relocatable (its .deb installs to /usr). The installed htgeom and
# cyberiadaml headers share include/cyberiada, already covered by INSTALL_INTERFACE
target_include_directories(cyberiadaml PUBLIC
				       $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			               $<INSTALL_INTERFACE:include>
			               $<INSTALL_INTERFACE:include/cyberiada>
				       "${LIBXML2_INCLUDE_DIR}"
				       $<BUILD_INTERFACE:${HTGeom_INCLUDE_DIR}>
				       $<BUILD_INTERFACE:${HTGeom_INCLUDE_ROOT}>)
# the found library paths are linked directly: the *_LIBRARY variables are
# files, not directories. The libxml2 list is expanded unquoted: on Windows it
# carries the optimized/debug keywords, which are read as such only when they
# arrive as separate arguments
# htgeom is linked by its absolute build-prefix path (build-only, so it is not
# baked into the export); at runtime libcyberiadaml.so pulls libhtgeom.so via its
# NEEDED entry, resolved from the install location (/usr/lib)
target_link_libraries(cyberiadaml PUBLIC
				  ${LIBXML2_LIBRARIES}
				  $<BUILD_INTERFACE:${HTGeom_LIBRARY}>)

# the POSIX regexps are not available outside Linux: use the PCRE2 wrapper
if(NOT CMAKE_SYSTEM_NAME STREQUAL Linux)
  find_path(PCRE2_POSIX_INCLUDE_DIR pcre2posix.h)
  find_library(PCRE2_POSIX_LIBRARY NAMES pcre2-posix pcre2-posix-static)
  if(NOT PCRE2_POSIX_INCLUDE_DIR OR NOT PCRE2_POSIX_LIBRARY)
    message(FATAL_ERROR "Cannot find the PCRE2 POSIX wrapper library")
  endif()
  target_include_directories(cyberiadaml PRIVATE "${PCRE2_POSIX_INCLUDE_DIR}")
  target_link_libraries(cyberiadaml PUBLIC "${PCRE2_POSIX_LIBRARY}")
endif()

target_compile_definitions(cyberiadaml PRIVATE
					   $<$<NOT:$<PLATFORM_ID:Linux>>:CYBERIADA_PCRE2_REGEXPS>)

add_subdirectory(parser)

set(MEMORYCHECK_COMMAND_OPTIONS "--tool=memcheck --leak-check=full --error-exitcode=1")
include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

install(TARGETS cyberiadaml EXPORT cyberiadaml
        RUNTIME DESTINATION bin COMPONENT runtime
        LIBRARY DESTINATION lib COMPONENT runtime
        ARCHIVE DESTINATION lib COMPONENT dev)
install(FILES cyberiadaml.h cyberiadaml_iso.h
        DESTINATION include/cyberiada
        COMPONENT dev)
install(EXPORT cyberiadaml DESTINATION lib/cmake/cyberiadaml
        COMPONENT dev)

configure_file(
   ${PROJECT_SOURCE_DIR}/cyberiadaml-config.cmake.in
   ${PROJECT_BINARY_DIR}/cyberiadaml-config.cmake @ONLY)

install(FILES ${PROJECT_BINARY_DIR}/cyberiadaml-config.cmake
	DESTINATION lib/cmake/cyberiadaml
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
