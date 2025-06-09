include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${MPI_CXX_INCLUDE_PATH}
)
set(SRCS
   SeapodymCohortFake.cpp
   SeapodymCohortManager.cpp
   SeapodymCourier.cpp
   CmdLineArgParser.cpp
   )

add_library(seapodym_api ${SRCS})
target_link_libraries(seapodym_api admb ${MPI_LIBRARIES})
