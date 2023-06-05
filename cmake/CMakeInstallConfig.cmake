include(CMakeFindDependencyMacro)

find_dependency(range-v3 REQUIRED)
find_dependency(fmt REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/proc_watcherTargets.cmake")
