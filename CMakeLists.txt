cmake_minimum_required(VERSION 3.25)
project(proc_watcher)

set(CMAKE_CXX_STANDARD 20)

# Set the project name and language
project(proc_watcher
        VERSION 1.0
        DESCRIPTION "Library to scan the /proc directory and get information about the processes running in the system."
        LANGUAGES CXX)

message(STATUS "Searching targets")
include(cmake/CMakeTargets.cmake)
message(STATUS "Searching targets - done")

message(STATUS "Configuring CMake variables")
include(cmake/CMakePreConf.cmake)
message(STATUS "Configuring CMake variables - done")

message(STATUS "Solving CMake options")
include(cmake/CMakeOptions.cmake)
message(STATUS "Solving CMake options - done")

message(STATUS "Searching external libraries")
include(cmake/CMakeLibs.cmake)
message(STATUS "Searching external libraries - done")

message(STATUS "Linking libraries")
include(cmake/CMakeLink.cmake)
message(STATUS "Linking libraries - done")

message(STATUS "Trying to enable Link Time Optimization (LTO)")
include(cmake/CMakeLTO.cmake)
message(STATUS "Trying to enable Link Time Optimization (LTO) - done")