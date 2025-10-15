# FindFmt.cmake
# This will define:
#   FMT_FOUND          - TRUE if found
#   FMT_INCLUDE_DIRS   - Where include files reside
#   FMT_LIBRARIES      - Library to link against (if needed)
#   fmt::fmt           - Imported target (for target_link_libraries)

# Typical locations:
# Ubuntu (apt):    /usr/include/fmt
# Mac (brew):      /usr/local/include/fmt or /opt/homebrew/include/fmt

# --- Find fmt include directory ---
find_path(
    FMT_INCLUDE_DIR
    NAMES fmt/core.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/homebrew/include
)

find_library(
    FMT_LIBRARY
    NAMES fmt
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/homebrew/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    fmt
    REQUIRED_VARS FMT_INCLUDE_DIR
    # Library is optional, so no REQUIRED_VARS for FMT_LIBRARY
)

if(FMT_FOUND)
    set(FMT_INCLUDE_DIRS ${FMT_INCLUDE_DIR})

    if(FMT_LIBRARY)
        set(FMT_LIBRARIES ${FMT_LIBRARY})
    else()
        set(FMT_LIBRARIES "")
    endif()

    if(NOT TARGET fmt::fmt)
        if(FMT_LIBRARY)
            add_library(fmt::fmt UNKNOWN IMPORTED)
            set_target_properties(fmt::fmt PROPERTIES
                IMPORTED_LOCATION "${FMT_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${FMT_INCLUDE_DIR}"
            )
        else()
            add_library(fmt::fmt INTERFACE IMPORTED)
            set_target_properties(fmt::fmt PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${FMT_INCLUDE_DIR}"
            )
        endif()
    endif()
endif()
