# Ranges
# From : https://alandefreitas.github.io/moderncpp/algorithms-data-structures/algorithm/ranges/
# Use range-v3 for now: https://github.com/ericniebler/range-v3
CPMAddPackage("gh:ericniebler/range-v3#0.12.0")
if (TARGET range-v3)
    message(STATUS "Found range-v3: ${range-v3_SOURCE_DIR}")
endif ()

if (TARGET tabulate)
    message(STATUS "Found tabulate: ${tabulate_SOURCE_DIR}")
endif ()

# spdlog
CPMAddPackage(
        NAME spdlog
        GITHUB_REPOSITORY "gabime/spdlog"
        VERSION 1.11.0
        OPTIONS "SPDLOG_BUILD_BENCH OFF" "SPDLOG_FMT_EXTERNAL ON" "SPDLOG_BUILD_SHARED ON" "SPDLOG_BUILD_TESTS OFF"
)

if (TARGET spdlog::spdlog_header_only)
    message(STATUS "Found spdlog_header_only: ${spdlog_SOURCE_DIR}")
endif ()
