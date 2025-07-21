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
set(HEADERS
   CmdLineArgParser.h
   SeapodymCohortFake.h
   SeapodymCourier.h
   SeapodymCohortAbstract.h  
   SeapodymCohortManager.h
)

add_library(seapodym_api ${SRCS})
target_link_libraries(seapodym_api admb ${MPI_LIBRARIES})
install(TARGETS seapodym_api DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include)
