cmake_minimum_required(VERSION 3.16)

set(CMAKE_VERBOSE_MAKEFILE ON)

project("proc_watcher_demo" CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME} main.cpp)

set(CPM_USE_LOCAL_PACKAGES ON)
set(CPM_USE_NAMED_CACHE_DIRECTORIES ON)
include(../cmake/get_cpm.cmake)

# spdlog
CPMAddPackage("gh:gabime/spdlog")
if (TARGET spdlog::spdlog)
    message(STATUS "Found spdlog: OK")
else ()
    message(SEND_ERROR "Found spdlog: ERROR")
endif ()

# CLI11
CPMAddPackage("gh:CLIUtils/CLI11@2.3.2")
if (TARGET CLI11::CLI11)
    message(STATUS "Found CLI11: OK")
else ()
    message(SEND_ERROR "Found CLI11: ERROR")
endif ()

# Range-v3
CPMAddPackage("gh:ericniebler/range-v3@0.12.0")
if (TARGET range-v3::range-v3)
    message(STATUS "Found range-v3: OK")
else ()
    message(SEND_ERROR "Found range-v3: ERROR")
endif ()

find_package(proc_watcher CONFIG REQUIRED)
if (TARGET proc_watcher::proc_watcher)
    message(STATUS "Found proc_watcher: OK")
else ()
    message(SEND_ERROR "Found proc_watcher: ERROR")
endif ()

if (TARGET proc_watcher::proc_watcher)
    target_link_libraries(proc_watcher_demo PRIVATE proc_watcher::proc_watcher)
endif ()

if (TARGET spdlog::spdlog)
    target_link_libraries(proc_watcher_demo PRIVATE spdlog::spdlog)
endif ()

if (TARGET CLI11::CLI11)
    target_link_libraries(proc_watcher_demo PRIVATE CLI11::CLI11)
endif ()

if (TARGET range-v3::range-v3)
    target_link_libraries(proc_watcher_demo PRIVATE range-v3::range-v3)
endif ()