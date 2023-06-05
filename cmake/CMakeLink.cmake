if (TARGET range-v3::range-v3)
    target_link_libraries(${PROJECT_NAME} INTERFACE range-v3::range-v3)
endif ()

if (TARGET fmt::fmt)
    target_link_libraries(${PROJECT_NAME} INTERFACE fmt::fmt)
endif ()

target_link_libraries(${PROJECT_NAME} INTERFACE numa)