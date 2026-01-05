# Minimum required version of CMake to process this file.
# This ensures compatibility; older CMake versions may not support some commands.
cmake_minimum_required(VERSION 2.6)

# Define the project name. This sets up some default variables like ${PROJECT_NAME}.
project(gpr)

# Set compiler flags for debug builds.
# "-g" tells the compiler to include debug information for debugging tools (like gdb).
set(CMAKE_CXX_FLAGS_DEBUG "-g")


# Enable verbose output during the build process.
# When this is ON, CMake generates Makefiles (or other build files) that
# print every command executed by the build system to the terminal.
# This includes:
#   - The exact compiler commands (g++, clang++, etc.)
#   - All compiler flags (like -std=c++11, -Wall, -O2, -g, etc.)
#   - Include directories (-I options)
#   - Linking commands for libraries and executables
#   - File paths of source files being compiled
#
# This is extremely useful for:
#   - Debugging build issues
#   - Seeing exactly what flags and options are being used
#   - Ensuring the correct compiler and paths are applied
#   - Understanding the full compilation/linking process
#
# Without this, CMake usually only prints short progress messages like:
#   [ 50%] Building CXX object src/parser.cpp.o
#
# With verbose ON, you'll see something like:
#   /usr/bin/g++ -std=c++11 -Wall -Werror -I./src -I./test -O2 -o src/parser.cpp.o -c src/parser.cpp
set(CMAKE_VERBOSE_MAKEFILE ON)


# Set compiler flags for release builds.
# "-O2" enables optimization for faster code at the expense of longer compilation.
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# Conditional compilation flags for GNU C++ compiler or Clang.
# CMAKE_COMPILER_IS_GNUCXX is true if GCC is used.
# "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" is true if Clang is used.
if(CMAKE_COMPILER_IS_GNUCXX OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
    # Additional flags for GCC or Clang:
    # -std=c++11   → Use C++11 standard features
    # -I./src      → Include directory for header files
    # -I./test     → Include directory for test files
    # -Werror      → Treat all warnings as errors (stops compilation on warnings)
    # -Wall        → Enable all common warnings
	SET(EXTRA_CXX_COMPILE_FLAGS "-std=c++11 -I./src -I./test -Werror -Wall")

    # Append these extra flags to existing compiler flags
	SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_COMPILE_FLAGS}")
endif(CMAKE_COMPILER_IS_GNUCXX OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))

# Include directories for the compiler to search for header files
# This tells CMake that headers in ./src/ are needed during compilation
INCLUDE_DIRECTORIES(./src/)

# List of header files for the library
# Used here mostly for documentation/IDE purposes; CMake doesn’t need headers to build the library
SET(GPR_HEADERS src/gcode_program.h
		src/parser.h)

# List of source files (.cpp) for the library
# These are the actual files compiled into the library
SET(GPR_CPPS src/parser.cpp
	     src/gcode_program.cpp)

# On macOS, allow RPATH to be set for shared libraries
# RPATH controls where the runtime loader looks for dynamic libraries
set(CMAKE_MACOSX_RPATH 1)

# Create a shared library called "gpr" using the listed .cpp files
# Shared library (.so on Linux, .dylib on macOS) can be linked by multiple programs
add_library(gpr SHARED ${GPR_CPPS})

# List of test source files
SET(TEST_FILES test/parser_tests.cpp
	       test/main.cpp)

# Create an executable for running all tests
# The executable will link to the "gpr" library defined above
add_executable(all-tests ${TEST_FILES})
target_link_libraries(all-tests gpr)

# Create another executable for the main G-code parsing program
# This also links to the "gpr" shared library
add_executable(parse-gcode src/main.cpp)
target_link_libraries(parse-gcode gpr)

# Install rules
# When running "make install", this will copy the "gpr" library to the system's lib directory
# LIBRARY DESTINATION → for shared libraries
install(TARGETS gpr LIBRARY DESTINATION lib)
