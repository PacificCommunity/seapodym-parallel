include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${MPI_CXX_INCLUDE_PATH}
)
set(SRCS
   DistDataCollector.cpp
   SeapodymCohortDependencyAnalyzer.cpp
   TaskStepManager.cpp
   TaskStepWorker.cpp
   TaskScore.cpp
   TaskDependencyManager.cpp
   TaskManager.cpp
   TaskWorker.cpp
   SeapodymCohortFake.cpp
   SeapodymCohortManager.cpp
   SeapodymCourier.cpp
   CmdLineArgParser.cpp
   )
set(HEADERS
   Tags.h
   DistDataCollector.h
   SeapodymCohortDependencyAnalyzer.h
   TaskStepManager.h
   TaskStepWorker.h
   TaskScore.h
   TaskDependencyManager.h
   TaskManager.h
   TaskWorker.h
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
