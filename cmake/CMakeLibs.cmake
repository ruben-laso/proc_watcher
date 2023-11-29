# Range-v3
find_package(range-v3 CONFIG REQUIRED)
if (TARGET range-v3::range-v3)
    message(STATUS "Found range-v3: OK")
else ()
    message(SEND_ERROR "Found range-v3: ERROR")
endif ()

# fmt
find_package(fmt CONFIG REQUIRED)
if (TARGET fmt::fmt)
    message(STATUS "Found fmt: OK")
else ()
    message(SEND_ERROR "Found fmt: ERROR")
endif ()