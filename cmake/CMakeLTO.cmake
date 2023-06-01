include(CheckIPOSupported)
check_ipo_supported(RESULT IPOsupported OUTPUT error)

if (IPOsupported)
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else ()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif ()