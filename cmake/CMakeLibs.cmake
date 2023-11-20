# Range-v3
CPMAddPackage("gh:ericniebler/range-v3#0.12.0")
if (TARGET range-v3::range-v3)
    message(STATUS "Found range-v3: OK")
else ()
    message(SEND_ERROR "Found range-v3: ERROR")
endif ()

# fmt
CPMAddPackage("gh:fmtlib/fmt#10.1.1")
if (TARGET fmt::fmt)
    message(STATUS "Found fmt: OK")
else ()
    message(SEND_ERROR "Found fmt: ERROR")
endif ()