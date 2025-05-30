cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(seapodym-parallel VERSION 0.1)

enable_testing()

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


find_package(MPI REQUIRED)

option(ADMB_HOME "Path to ADMB installation" "")
# If ADMB_HOME is set, use it to configure include and library paths
if(ADMB_HOME)
    message(STATUS "Using ADMB from: ${ADMB_HOME}")
    include_directories(${ADMB_HOME}/include)
    link_directories(${ADMB_HOME}/lib)
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
