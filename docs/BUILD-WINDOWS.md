# Building the CyberiadaML C/C++ Stack on Windows

This document describes how to build the three C/C++ libraries on Windows with
CMake and VS Code. The stack is developed on Linux; the Windows build works
through the same CMake files, but the dependencies are installed differently
and the tests are run by CTest instead of the shell wrappers.

```
    libhtreegeom          libcyberiadaml            libcyberiadamlpp
   +--------------+      +-----------------+      +------------------+
   |  htgeom.dll  | <--- |  cyberiadaml.dll| <--- | cyberiadamlpp.dll|
   |  homog2d.hpp |      |  libxml2, pcre2 |      |                  |
   +--------------+      +-----------------+      +------------------+
        build 1                build 2                  build 3
```

Each library is a separate CMake project and must be built and installed
before the next one: the arrows show the dependency direction.

## Prerequisites

* Visual Studio Build Tools 2022 (the "Desktop development with C++" workload:
  the MSVC compiler and the Windows SDK);
* CMake 3.21 or newer (the Build Tools installer provides it);
* Git for Windows;
* VS Code with the **C/C++** and **CMake Tools** extensions;
* [vcpkg](https://vcpkg.io) for the third-party libraries.

## The dependencies

Install the third-party libraries with vcpkg once:

    git clone https://github.com/microsoft/vcpkg
    .\vcpkg\bootstrap-vcpkg.bat
    .\vcpkg\vcpkg install libxml2:x64-windows pcre2:x64-windows

`libhtreegeom` additionally needs the header-only
[homog2d](https://github.com/skramm/homog2d) library: download `homog2d.hpp`
into the root of the `libhtreegeom` source directory (on Linux the file is
usually a symbolic link; on Windows it must be a regular file).

`libcyberiadaml` uses the POSIX regular expressions on Linux and the PCRE2
POSIX wrapper everywhere else, so `pcre2` is required on Windows.

## Building in VS Code

1. Open the library folder (`File > Open Folder`).
2. Point CMake Tools at vcpkg and at the previously built libraries. Create
   `.vscode/settings.json`:

        {
          "cmake.configureSettings": {
            "CMAKE_TOOLCHAIN_FILE": "C:/vcpkg/scripts/buildsystems/vcpkg.cmake",
            "CMAKE_PREFIX_PATH": "C:/cyberiada",
            "CMAKE_INSTALL_PREFIX": "C:/cyberiada"
          }
        }

3. Press `Ctrl+Shift+P` > **CMake: Select a Kit** and choose
   *Visual Studio Build Tools 2022 - amd64*.
4. **CMake: Configure**, then **CMake: Build** (or `F7`).
5. Run the tests from the **Testing** panel, or with **CMake: Run Tests**.
6. Install with **CMake: Install** before building the next library.

Repeat for the three libraries in the order shown in the scheme above.

## The same from the command line

Run from the *Developer Command Prompt for VS 2022*:

    cmake -B build -G "Visual Studio 17 2022" -A x64 ^
          -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
          -DCMAKE_PREFIX_PATH=C:/cyberiada ^
          -DCMAKE_INSTALL_PREFIX=C:/cyberiada
    cmake --build build --config Release
    ctest --test-dir build -C Release --output-on-failure
    cmake --install build --config Release

The Visual Studio generator is a multi-configuration one: the configuration is
chosen at build time by `--config`, not by `CMAKE_BUILD_TYPE`.

`libhtreegeom` is found through its `FindHTGeom.cmake` module, which searches
Unix paths only; on Windows pass the location explicitly:

    -DHTGeom_INCLUDE_DIR=C:/cyberiada/include/cyberiada
    -DHTGeom_LIBRARY=C:/cyberiada/lib/htgeom.lib

`libcyberiadamlpp` finds `libcyberiadaml` through its CMake package
configuration; pass `-Dcyberiadaml_DIR=C:/cyberiada/lib/cmake` if it is
installed outside `CMAKE_PREFIX_PATH`.

## Running the programs

A DLL is searched next to the executable that uses it, so `cybparser.exe` and
`cyberiadapp.exe` need `htgeom.dll`, `cyberiadaml.dll`, `cyberiadamlpp.dll`
and the vcpkg DLLs (`libxml2.dll`, `pcre2-*.dll` and their dependencies) in
the same directory or in `PATH`. The install step puts the DLLs into
`<prefix>/bin` and the import libraries into `<prefix>/lib`; the test
executables get their DLLs copied automatically by the build.

## Packaging

The `cpack` configuration of the projects describes Debian packages and is
used on Linux only. On Windows build an archive instead:

    cpack -G ZIP -C Release

## MinGW-w64

The MSYS2 / MinGW-w64 toolchain also works and is closer to the Linux build.
Install the dependencies with `pacman` (`mingw-w64-x86_64-libxml2`,
`mingw-w64-x86_64-pcre2`, `mingw-w64-x86_64-cmake`), select the
*GCC for MinGW* kit in VS Code, and use the `Ninja` or `MinGW Makefiles`
generator. Note that the resulting binaries depend on the MinGW runtime and
cannot be mixed with MSVC-built ones in a single process.

## Known limitations

* `run-tests.sh` and `run-mem-tests.sh` need a POSIX shell; on Windows use
  `ctest` (the wrappers add nothing besides invoking it).
* The `18-key-remap` test of `libcyberiadaml` uses POSIX threads and is
  excluded from the build under MSVC.
* Valgrind-based memory testing (`-DMEMCHECK=ON`) is not available.
* The MSVC build is not covered by continuous integration: the sources and the
  CMake files were made Windows-compatible by review, and the Linux build and
  the test suites are verified, but the MSVC compilation itself has not been
  run by the authors. Please report the problems you meet.
