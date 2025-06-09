#include <SeapodymCohortFake.h>
#include <SeapodymCohortManager.h>
#include <SeapodymCourier.h>
#include <CmdLineArgParser.h>
#include <mpi.h>
#include <admodel.h>
#include <iostream>

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);
    int numWorkers;
    MPI_Comm_size(MPI_COMM_WORLD, &numWorkers);
    int workerId;
    MPI_Comm_rank(MPI_COMM_WORLD, &workerId);
    
    CmdLineArgParser cmdLine;
    cmdLine.set("-na", 3, "Number of age groups");
    cmdLine.set("-nt", 5, "Total number of time steps");
    cmdLine.set("-nm", 15, "Milliseconds per step");
    cmdLine.set("-nd", 3, "Number of doubles in the data array");
    bool success = cmdLine.parse(argc, argv);
    bool help = cmdLine.get<bool>("-help") || cmdLine.get<bool>("-h");
    if (!success) {
        std::cerr << "Error parsing command line arguments." << std::endl;
    }
    if (help) {
        cmdLine.help();
        MPI_Finalize();
        return 1;
    }

    // Number of age groups, time steps, milliseconds per step, and number of doubles
    // These values are set by the command line arguments
    int numAgeGroups = cmdLine.get<int>("-na");
    int nt = cmdLine.get<int>("-nt");
    int nm = cmdLine.get<int>("-nm");
    int nd = cmdLine.get<int>("-nd");
    if (numAgeGroups <= 0 || nt <= 0 || nm <= 0 || nd <= 0) {
        if (workerId == 0) {
            // Print an error message only from the master worker
            std::cerr << "Invalid command line arguments. All values must be positive integers." << std::endl;
        }
        cmdLine.help();
        MPI_Finalize();
        return 1;
    }
    if (numAgeGroups <= 0 || nt <= 0 || numAgeGroups < numWorkers) {
        if (workerId == 0) {
            // Print an error message only from the master worker
            std::cerr << "Invalid number of age groups (" << numAgeGroups << "), time steps (" << nt << ") or workers (" << numWorkers << ").\n";
            std::cerr << "Number of age groups must be positive and at least equal to the number of workers." << std::endl;
            cmdLine.help();
        }
        MPI_Finalize();
        return 2;
    }

    if (workerId == 0) {
        std::cout << "Running with " << numWorkers << " workers." << std::endl;
        std::cout << "Number of age groups: " << numAgeGroups << ", Total number of time steps: " << nt << std::endl;
    }

    SeapodymCohortManager cohortManager(numAgeGroups, numWorkers, nt);
    SeapodymCourier courier(MPI_COMM_WORLD);

    std::vector<double> data(nd, workerId);
    courier.expose(data.data(), nd);

    std::vector<int> cohortIds = cohortManager.getInitCohortIds(workerId);

    // Initialize the step counter for each cohort task
    std::vector<int> step_counter(cohortIds.size(), 0);

    // Initialize the number of steps for each cohort task
    std::vector<int> cohortNumSteps(cohortIds.size());
    std::vector<SeapodymCohortFake*> cohortsPerWorker(cohortIds.size());
    for (auto i = 0; i < cohortIds.size(); ++i) {
        cohortNumSteps[i] = cohortManager.getNumSteps(cohortIds[i]);
        SeapodymCohortFake* cohortPtr = new SeapodymCohortFake(nm, nd, cohortIds[i]);
        cohortsPerWorker[i] = cohortPtr;
    }

    // Iterate over the global time steps
    for (auto istep = 0; istep < nt; ++istep) {

        // Iterate over the cohort tasks assigned to this worker
        for (auto icohort = 0; icohort < cohortIds.size(); ++icohort) {
        
            // Get the cohort Id
            int cohortId = cohortIds[icohort];

            // Get the number of steps for this cohort task
            int numSteps = cohortNumSteps[icohort];

            std::cout <<   "Worker " << workerId << " processing cohort " << cohortId 
                      << " at time step " << istep << " with " << numSteps - step_counter[icohort] << " remaining steps." << std::endl;  
            
            // Simulate the time taken for a step
            cohortsPerWorker[icohort]->stepForward(dvar_vector());
            
            // Done with the step
            step_counter[icohort]++;

            // Find out whether we have to switch to another cohort task
            if (step_counter[icohort] >= numSteps) {
                // Switch over to a new cohort task, reset the step counter and the number of steps
                cohortIds[icohort] = cohortManager.getNextCohort(cohortId);
                step_counter[icohort] = 0;
                cohortNumSteps[icohort] = cohortManager.getNumSteps(cohortIds[icohort]);

                // Accumulate the data from all workers
                std:vector<double> sum_data(nd, 0.0);
                // Could be using MPI_Accumulate instead
                for (auto iw = 0; iw < numWorkers; ++iw) {
                    if (iw != workerId) {
                        std::vector<double> fetchedData = courier.fetch(iw);
                        std::cout << "Worker " << workerId << " fetched data from worker " << iw << ": ";
                        for (int i = 0; i < nd; ++i) {
                            std::cout << fetchedData[i] << " ";
                            sum_data[i] += fetchedData[i]; // Sum the data from all workers
                        }
                        std::cout << std::endl;
                    }
                }
            }
        }

    }

    for (auto cohortPtr : cohortsPerWorker) {
        if (cohortPtr) {
            delete cohortPtr; // Free the memory allocated for the cohort
        }
    }

    courier.free(); // Free the MPI window


    MPI_Finalize();
    return 0;
}
