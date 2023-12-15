add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set_property(
        TARGET ${PROJECT_NAME} PROPERTY
        EXPORT_NAME ${PROJECT_NAME}
)

target_include_directories(
        ${PROJECT_NAME} ${warning_guard}
        INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
