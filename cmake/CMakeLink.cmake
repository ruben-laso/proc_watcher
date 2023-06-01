if (TARGET range-v3)
    target_link_libraries(${PROJECT_NAME} PRIVATE range-v3)
endif ()

if (TARGET spdlog::spdlog_header_only)
    target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog_header_only)
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE numa)