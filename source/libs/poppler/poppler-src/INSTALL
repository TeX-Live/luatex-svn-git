Installation Instructions
*************************

Basic Installation
==================

mkdir build
cd build
cmake ..
make
make install


CMake configuration options can be set using the -D option. eg

  cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=release


Build Options
=============

Set install prefix:

  -DCMAKE_INSTALL_PREFIX=<path>

Set build type. This sets the standard compiler flags for the build
type.

  -DCMAKE_BUILD_TYPE=debug  or  -DCMAKE_BUILD_TYPE=release

Set compiler flags:

  -DCMAKE_CXX_FLAGS=<flags>   or set CXXFLAGS environment variable

Set linker flags:

  -DCMAKE_LD_FLAGS=<flags>   or set LDFLAGS environment variable


Optional Features
=================

  -D<FEATURE>=<ON|OFF>

eg

  -DENABLE_SPLASH=ON -DBUILD_GTK_TESTS=OFF

A list of all options can be display with the commmand:

  egrep '^ *(option|set.*STRING)' CMakeLists.txt

Alternatively, the options can be edited by running "ccmake ." in the
build directory.


Cross Compiling
===============

A toolchain file is required to specify the target specific compiler
tools. Run cmake with the option:

  -DCMAKE_TOOLCHAIN_FILE=<Toolchain file>

A sample toolchain for a 64-bit mingw build is shown below. Replace
/path/to/win/root with the install prefix for the target environment.

  SET(CMAKE_SYSTEM_NAME Windows)
  SET(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
  SET(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
  SET(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
  SET(CMAKE_FIND_ROOT_PATH  /usr/x86_64-w64-mingw32 /path/to/win/root )
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

