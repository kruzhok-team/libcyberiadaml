# The Cyberiada State Machine Library

The C library for processing CyberiadaML - the version of GraphML for storing state machine graphs
used by the Cyberiada Project, the Berloga Project games and the Orbita Simulator. 

The code is distributed under the Lesser GNU Public License (version 3), the documentation -- under
the GNU Free Documentation License (version 1.3).

## Documentation

* [The library architecture](docs/ARCHITECTURE.md)
* [The API reference](docs/API.md)

## Requirements

* build-essential
* libxml2-dev
* [libhtreegeom](https://github.com/kruzhok-team/libhtreegeom)
* cmake (version 3.12+)
* libpcre2-dev (on Windows and macOS)

## Installation

Create `build` directory: `mkdir build && cd build`

Run `cmake ..` and `make` to build the library binaries and the `cybparser` command line tool.

Run `make install` to install the library.

Use CMake parameters to change the build type / installation prefix / etc.

## Testing

Build the library and run `ctest` in the `build` directory. Run
`ctest -T memcheck` to check the tests under valgrind.
