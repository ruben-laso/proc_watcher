# Debug and profile options
if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    MESSAGE(STATUS "Using compiler debug and profile flags: -g -pg -fno-omit-frame-pointer -fsanitize=address,undefined")
    set(CUSTOM_DEBUG_OPTIONS "-g -pg -fno-omit-frame-pointer -fsanitize=address,undefined")
    set(CUSTOM_DEBUG_LINK_FLAGS "-pg -lasan -fsanitize=address,undefined")

    target_compile_options(${PROJECT_NAME} INTERFACE <$<CONFIG:DEBUG>:${CUSTOM_DEBUG_OPTIONS}>)
    target_compile_options(${PROJECT_NAME} INTERFACE <$<CONFIG:RELWITHDEBINFO>:${CUSTOM_DEBUG_OPTIONS}>)

    target_link_libraries(${PROJECT_NAME} INTERFACE <$<CONFIG:DEBUG>:${CUSTOM_DEBUG_LINK_FLAGS}>)
    target_link_libraries(${PROJECT_NAME} INTERFACE <$<CONFIG:RELWITHDEBINFO>:${CUSTOM_DEBUG_LINK_FLAGS}>)
endif ()

MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

MESSAGE(STATUS "Compiler family: ${CMAKE_CXX_COMPILER_ID}")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # using Clang
    MESSAGE(STATUS "Using further compiler flags: -fconcepts")
    target_compile_options(${PROJECT_NAME} INTERFACE <$<CONFIG:DEBUG>:-fconcepts>)
    target_compile_options(${PROJECT_NAME} INTERFACE <$<CONFIG:RELEASE>:-fconcepts>)
    target_compile_options(${PROJECT_NAME} INTERFACE <$<CONFIG:RELWITHDEBINFO>:-fconcepts>)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # using Intel C++
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    MESSAGE(STATUS "Compilation flags: ${CMAKE_CXX_FLAGS_RELEASE}")
    MESSAGE(STATUS "Using further compiler optimisation flags: -march=native -ffast-math")

    set(CUSTOM_OPT_FLAGS "-march=native -mtune=native -ffast-math")

    target_compile_options(${PROJECT_NAME} INTERFACE <$<CONFIG:RELEASE>:${CUSTOM_OPT_FLAGS}>)
endif ()