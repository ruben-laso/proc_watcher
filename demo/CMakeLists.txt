cmake_minimum_required(VERSION 3.16)

set(CMAKE_VERBOSE_MAKEFILE ON)

project("proc_watcher_demo" CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME} main.cpp)

set(CPM_USE_LOCAL_PACKAGES ON)
set(CPM_USE_NAMED_CACHE_DIRECTORIES ON)
include(../cmake/get_cpm.cmake)

# fmt
CPMAddPackage("gh:fmtlib/fmt")
if (TARGET fmt::fmt)
    message(STATUS "Found fmt: OK")
else ()
    message(SEND_ERROR "Found fmt: ERROR")
endif ()

# spdlog
CPMAddPackage("gh:gabime/spdlog")
if (TARGET spdlog::spdlog)
    message(STATUS "Found spdlog: OK")
else ()
    message(SEND_ERROR "Found spdlog: ERROR")
endif ()

find_package(proc_watcher CONFIG REQUIRED)
if (TARGET proc_watcher::proc_watcher)
    message(STATUS "Found proc_watcher: OK")
else ()
    message(SEND_ERROR "Found proc_watcher: ERROR")
endif ()


if (TARGET fmt::fmt)
    target_link_libraries(proc_watcher_demo PRIVATE fmt::fmt)
endif ()

if (TARGET proc_watcher::proc_watcher)
    target_link_libraries(proc_watcher_demo PRIVATE proc_watcher::proc_watcher)
endif ()

if (TARGET spdlog::spdlog)
    target_link_libraries(proc_watcher_demo PRIVATE spdlog::spdlog)
endif ()