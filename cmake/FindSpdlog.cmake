# FindSpdlog.cmake
# This will define:
#   SPDLOG_FOUND          - TRUE if found
#   SPDLOG_INCLUDE_DIRS   - Where spdlog/spdlog.h resides
#   SPDLOG_LIBRARIES      - Library to link against (if needed)
# And it creates:
#   spdlog::spdlog        - Imported target

# Typical locations:
# Ubuntu (apt):    /usr/include/spdlog
# Mac (brew):      /usr/local/include/spdlog or /opt/homebrew/include/spdlog

find_path(
    SPDLOG_INCLUDE_DIR
    NAMES spdlog/spdlog.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/homebrew/include
)

# spdlog is typically header-only, but can also be built as a library.
# Try to find a library just in case the user installed it that way.
find_library(
    SPDLOG_LIBRARY
    NAMES spdlog
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/homebrew/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Spdlog
    REQUIRED_VARS SPDLOG_INCLUDE_DIR
    # Library is optional, so no REQUIRED_VARS for SPDLOG_LIBRARY
)

if(SPDLOG_FOUND)
    set(SPDLOG_INCLUDE_DIRS ${SPDLOG_INCLUDE_DIR})

    if(SPDLOG_LIBRARY)
        set(SPDLOG_LIBRARIES ${SPDLOG_LIBRARY})
    else()
        # header-only
        set(SPDLOG_LIBRARIES "")
    endif()

    if(NOT TARGET spdlog::spdlog)
        if(SPDLOG_LIBRARY)
            add_library(spdlog::spdlog UNKNOWN IMPORTED)
            set_target_properties(spdlog::spdlog PROPERTIES
                IMPORTED_LOCATION "${SPDLOG_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${SPDLOG_INCLUDE_DIR}"
            )
        else()
            add_library(spdlog::spdlog INTERFACE IMPORTED)
            set_target_properties(spdlog::spdlog PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${SPDLOG_INCLUDE_DIR}"
            )
        endif()
    endif()
endif()
