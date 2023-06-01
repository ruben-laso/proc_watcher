file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp)
add_executable(${PROJECT_NAME} ${sources})