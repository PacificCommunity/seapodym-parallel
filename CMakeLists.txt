cmake_minimum_required(VERSION 3.14)

# Set the project name and version
project(seapodym-parallel VERSION 0.1)

enable_testing()

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(MPI REQUIRED)
find_program(VALGRIND_EXECUTABLE valgrind)

option(FETCH "Whether to fetch data individually from each cohort and sum the result (ON), or just accumulate (OFF)" OFF)
if (FETCH)
    message(STATUS "Fetching data individually from each cohort.")
    add_definitions(-DSEAPODYM_FETCH)
endif ()

set(ADMB_HOME "" CACHE PATH "Path to ADMB installation")
# If ADMB_HOME is set, use it to configure include and library paths
if(ADMB_HOME)
    message(STATUS "Using ADMB from: ${ADMB_HOME}")

    # expand ~
    set(HOME_DIR "$ENV{HOME}")
    string(REGEX REPLACE "^~" "${HOME_DIR}" ADMB_HOME "${ADMB_HOME}")
    message(STATUS "Using ADMB from: ${ADMB_HOME}")

    # make sure we can find what we need
    set(ADMB_INCLUDE_DIR "")
    set(ADMB_LIBRARY "")
    if(EXISTS "${ADMB_HOME}/include/admodel.h")
        message(STATUS "${ADMB_HOME}/include/admodel.h found")
        set(ADMB_INCLUDE_DIR "${ADMB_HOME}/include")
    else()
        message(FATAL_ERROR "Could not find ${ADMB_HOME}/include/admodel.h")
    endif()
    if (EXISTS "${ADMB_HOME}/lib/libadmb.a")
        message(STATUS "${ADMB_HOME}/lib/libadmb.a found")
        set(ADMB_LIBRARY "${ADMB_HOME}/lib/libadmb.a")
    elseif(EXISTS "${ADMB_HOME}/lib/libadmb.so")
        message(STATUS "${ADMB_HOME}/lib/libadmb.so found")
        set(ADMB_LIBRARY "${ADMB_HOME}/lib/libadmb.so")
    else()
        message(FATAL_ERROR "Could not find ${ADMB_HOME}/lib/libadmb.{a,so}")
    endif()

    add_library(admb INTERFACE)
    target_include_directories(admb INTERFACE "${ADMB_INCLUDE_DIR}")
    target_link_libraries(admb INTERFACE "${ADMB_LIBRARY}")
else()
    message(FATAL_ERROR "ADMB_HOME not set. Please specify -DADMB_HOME=/path/to/admb")
endif()

find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    add_custom_target(doc_doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_IN}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(scripts)
