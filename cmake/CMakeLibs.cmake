# Range-v3
find_package(range-v3 CONFIG REQUIRED)
if (TARGET range-v3::range-v3)
    message(STATUS "Found range-v3: OK")
    target_link_libraries(${PROJECT_NAME} INTERFACE range-v3::range-v3)
else ()
    message(SEND_ERROR "Found range-v3: ERROR")
endif ()

# fmt
find_package(fmt CONFIG REQUIRED)
if (TARGET fmt::fmt)
    message(STATUS "Found fmt: OK")
    target_link_libraries(${PROJECT_NAME} INTERFACE fmt::fmt)
else ()
    message(SEND_ERROR "Found fmt: ERROR")
endif ()

target_link_libraries(${PROJECT_NAME} INTERFACE numa)