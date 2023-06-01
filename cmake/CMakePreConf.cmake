# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
set(CMAKE_CXX_EXTENSIONS ON)

# Additional (pedantic) compilation errors
set(PEDANTIC_FLAGS "-Wall -Wextra -Wshadow -Wconversion -Wpedantic -Werror -Wno-unused-function")
target_compile_options(${PROJECT_NAME} INTERFACE "$<$<CONFIG:DEBUG>:${PEDANTIC_FLAGS}>")
target_compile_options(${PROJECT_NAME} INTERFACE "$<$<CONFIG:RELEASE>:${PEDANTIC_FLAGS}>")
target_compile_options(${PROJECT_NAME} INTERFACE "$<$<CONFIG:RELWITHDEBINFO>:${PEDANTIC_FLAGS}>")

# CPM Setup
set(CPM_USE_LOCAL_PACKAGES ON)
set(CPM_USE_NAMED_CACHE_DIRECTORIES ON)
include(cmake/get_cpm.cmake)

# Add our module directory to the include path.
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

include_directories(src/)
include_directories(inc/)