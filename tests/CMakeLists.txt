include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src
  ${MPI_CXX_INCLUDE_PATH}
)

# Add the test executables
configure_file(testSeapodymCohort.cpp testSeapodymCohort.cxx)
add_executable(testSeapodymCohort testSeapodymCohort.cxx)
target_link_libraries(testSeapodymCohort PRIVATE seapodym_api)

add_executable(testSeapodymCohortManager testSeapodymCohortManager.cxx)
target_link_libraries(testSeapodymCohortManager PRIVATE seapodym_api)

add_executable(testSeapodymCohortManagerFake testSeapodymCohortManagerFake.cxx)
target_link_libraries(testSeapodymCohortManagerFake PRIVATE seapodym_api)

add_executable(testSeapodymCourier testSeapodymCourier.cxx)
target_link_libraries(testSeapodymCourier PRIVATE seapodym_api)

add_executable(testParallel testParallel.cxx)
target_link_libraries(testParallel PRIVATE seapodym_api spdlog::spdlog)


# Register the test
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
