cmake_minimum_required(VERSION 3.14)

set(namespace "prox")

# Set the project name and language
project(prox
        VERSION 1.0
        DESCRIPTION "Library to scan the /proc directory and get information about the processes running in the system."
        LANGUAGES CXX)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

message(STATUS "Defining targets")
include(cmake/CMakeTargets.cmake)
message(STATUS "Defining targets - done")

message(STATUS "Resolving dependencies")
include(cmake/CMakeLibs.cmake)
message(STATUS "Resolving dependencies - done")

message(STATUS "Configuring install")
if (NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/install-rules.cmake)
endif ()
message(STATUS "Configuring install - done")

# ---- Examples ----

option(BUILD_EXAMPLES "Build example(s)" OFF)
if (PROJECT_IS_TOP_LEVEL)
    set(BUILD_EXAMPLES "${prox_DEVELOPER_MODE}")
endif ()

if (BUILD_EXAMPLES)
    add_subdirectory(example)
endif ()

# ---- Developer mode ----

if (NOT ${PROJECT_NAME}_DEVELOPER_MODE)
    return()
elseif (NOT PROJECT_IS_TOP_LEVEL)
    message(
            AUTHOR_WARNING
            "Developer mode is intended for developers of ${PROJECT_NAME}"
    )
endif ()

include(cmake/dev-mode.cmake)