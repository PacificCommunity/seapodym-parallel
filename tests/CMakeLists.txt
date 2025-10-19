include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src
  ${MPI_CXX_INCLUDE_PATH}
)

# Add the test executables
configure_file(testSeapodymCohort.cpp testSeapodymCohort.cxx)
add_executable(testSeapodymCohort testSeapodymCohort.cxx)
target_link_libraries(testSeapodymCohort PRIVATE seapodym_api)

add_executable(testSeapodymCohortDependencyAnalyzer testSeapodymCohortDependencyAnalyzer.cxx)
target_link_libraries(testSeapodymCohortDependencyAnalyzer PRIVATE seapodym_api)

add_executable(testSeapodymCohortManager testSeapodymCohortManager.cxx)
target_link_libraries(testSeapodymCohortManager PRIVATE seapodym_api)

add_executable(testSeapodymCohortManagerFake testSeapodymCohortManagerFake.cxx)
target_link_libraries(testSeapodymCohortManagerFake PRIVATE seapodym_api)

add_executable(testSeapodymCourier testSeapodymCourier.cxx)
target_link_libraries(testSeapodymCourier PRIVATE seapodym_api)

add_executable(testParallel testParallel.cxx)
target_link_libraries(testParallel PRIVATE seapodym_api spdlog::spdlog fmt::fmt)

add_executable(testTaskFarming testTaskFarming.cxx)
target_link_libraries(testTaskFarming PRIVATE seapodym_api)

add_executable(testTaskDepFarming testTaskDepFarming.cxx)
target_link_libraries(testTaskDepFarming PRIVATE seapodym_api)

add_executable(testTaskStepFarming testTaskStepFarming.cxx)
target_link_libraries(testTaskStepFarming PRIVATE seapodym_api spdlog::spdlog fmt::fmt)

add_executable(testTaskStepFarmingCohort testTaskStepFarmingCohort.cxx)
target_link_libraries(testTaskStepFarmingCohort PRIVATE seapodym_api spdlog::spdlog fmt::fmt)

add_executable(testDistDataCollector testDistDataCollector.cxx)
target_link_libraries(testDistDataCollector PRIVATE seapodym_api)

add_executable(testAsyncPutGet testAsyncPutGet.cxx)
target_link_libraries(testAsyncPutGet PRIVATE seapodym_api)

add_executable(testTaskScore testTaskScore.cxx)
target_link_libraries(testTaskScore PRIVATE seapodym_api)

# Register the tests

add_test(NAME testSeapodymCohort COMMAND testSeapodymCohort)

add_test(NAME testSeapodymCohortManager COMMAND testSeapodymCohortManager)

if (VALGRIND_EXECUTABLE)
  add_test(NAME testSeapodymCohortManagerFake COMMAND ${VALGRIND_EXECUTABLE} --leak-check=full --error-exitcode=1 ./testSeapodymCohortManagerFake)
  set_tests_properties(testSeapodymCohortManagerFake
    PROPERTIES PASS_REGULAR_EXPRESSION "ERROR SUMMARY: 0 errors")
else()
  add_test(NAME testSeapodymCohortManagerFake COMMAND testSeapodymCohortManagerFake)
endif()

add_test(NAME testSeapodymCourier2 COMMAND mpiexec -n 2 testSeapodymCourier)

add_test(NAME testParallel_nw3_na3_nt5 COMMAND mpiexec -n 3 testParallel -na 3 -nt 5)
set_tests_properties(testParallel_nw3_na3_nt5 PROPERTIES
  TIMEOUT 5  # Timeout in seconds
)

add_test(NAME testParallel_nw2_na3_nt5 COMMAND mpiexec -n 2 testParallel -na 3 -nt 5)
set_tests_properties(testParallel_nw2_na3_nt5 PROPERTIES
  TIMEOUT 5 # Timeout in seconds
)
add_test(NAME testParallel_nw4_na4_nt7 COMMAND mpiexec -n 4 testParallel -na 4 -nt 7 -nd 1)
set_tests_properties(testParallel_nw4_na4_nt7 PROPERTIES
                     PASS_REGULAR_EXPRESSION 
                     "step: 1 checksum: 604.*step: 2 checksum: 608.*step: 3 checksum: 612.*step: 4 checksum: 616.*step: 5 checksum: 620.*step: 6 checksum: 624"
)

add_test(NAME testParallel_nw3_na4_nt7 COMMAND mpiexec -n 4 testParallel -na 4 -nt 7 -nd 1)
set_tests_properties(testParallel_nw3_na4_nt7 PROPERTIES
                     FAIL_REGULAR_EXPRESSION "checksum: \\[^3\\]")

add_test(NAME testTaskFarming10 COMMAND mpiexec -n 4 testTaskFarming -nT 10)
set_tests_properties(testTaskFarming10 PROPERTIES PASS_REGULAR_EXPRESSION "Success")

add_test(NAME testTaskDepFarmingNa3Nt6 COMMAND mpiexec -n 4 testTaskDepFarming -na 3 -nt 6)
set_tests_properties(testTaskDepFarmingNa3Nt6 PROPERTIES PASS_REGULAR_EXPRESSION "Success")

add_test(NAME testTaskStepFarmingNT16Ns10 COMMAND mpiexec -n 5 testTaskStepFarming -nT 16 -ns 10)
set_tests_properties(testTaskStepFarmingNT16Ns10 PROPERTIES PASS_REGULAR_EXPRESSION "Success")

add_test(NAME testTaskStepFarmingCohortNa3Nt5 COMMAND mpiexec -n 5 testTaskStepFarmingCohort -na 3 -nt 5)
set_tests_properties(testTaskStepFarmingCohortNa3Nt5 PROPERTIES PASS_REGULAR_EXPRESSION "checksum: 450")

add_test(NAME testTaskStepFarmingCohortNa1Nt2 COMMAND mpiexec -n 2 testTaskStepFarmingCohort -na 1 -nt 2)
set_tests_properties(testTaskStepFarmingCohortNa1Nt2 PROPERTIES PASS_REGULAR_EXPRESSION "checksum: 10000")

add_test(NAME testTaskStepFarmingCohortNa5Nt10 COMMAND mpiexec -n 6 testTaskStepFarmingCohort -na 5 -nt 10 -nd 100000 -nm 1)
set_tests_properties(testTaskStepFarmingCohortNa5Nt10 PROPERTIES PASS_REGULAR_EXPRESSION "checksum: 32500000")

add_test(NAME testTaskStepFarmingCohortNa5Nt10Nw3 COMMAND mpiexec -n 4 testTaskStepFarmingCohort -na 5 -nt 10 -nd 100000 -nm 1)
set_tests_properties(testTaskStepFarmingCohortNa5Nt10Nw3 PROPERTIES PASS_REGULAR_EXPRESSION "checksum: 32500000")

add_test(NAME testTaskScore COMMAND mpiexec -n 3 testTaskScore)
set_tests_properties(testTaskScore PROPERTIES PASS_REGULAR_EXPRESSION "SUCCEEDED")

add_test(NAME testSeapodymCohortDependencyAnalyzerNa3Nt5 COMMAND testSeapodymCohortDependencyAnalyzer -na 3 -nt 5)
set_tests_properties(testSeapodymCohortDependencyAnalyzerNa3Nt5 PROPERTIES PASS_REGULAR_EXPRESSION "Success")

add_test(NAME testSeapodymCohortDependencyAnalyzerNa4Nt5 COMMAND testSeapodymCohortDependencyAnalyzer -na 4 -nt 5)
set_tests_properties(testSeapodymCohortDependencyAnalyzerNa4Nt5 PROPERTIES PASS_REGULAR_EXPRESSION "Success")

add_test(NAME testDistDataCollector COMMAND mpiexec -n 2 testDistDataCollector)
set_tests_properties(testDistDataCollector PROPERTIES PASS_REGULAR_EXPRESSION "Success")

add_test(NAME testAsyncPutGet COMMAND mpiexec -n 6 testAsyncPutGet -nd 100000 -nm 100)
set_tests_properties(testAsyncPutGet PROPERTIES PASS_REGULAR_EXPRESSION "Success")
