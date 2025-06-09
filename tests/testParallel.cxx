#include <SeapodymCohortFake.h>
#include <SeapodymTaskManager.h>
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

    SeapodymTaskManager taskManager(numAgeGroups, numWorkers, nt);
    SeapodymCourier courier(MPI_COMM_WORLD);

    std::vector<double> data(nd, workerId);
    courier.expose(data.data(), nd);

    std::vector<int> taskIds = taskManager.getInitTaskIds(workerId);

    // Initialize the step counter for each task
    std::vector<int> step_counter(taskIds.size(), 0);

    // Initialize the number of steps for each task
    std::vector<int> taskNumSteps(taskIds.size());
    std::vector<SeapodymCohortFake*> cohortsPerWorker(taskIds.size());
    for (auto i = 0; i < taskIds.size(); ++i) {
        taskNumSteps[i] = taskManager.getNumSteps(taskIds[i]);
        SeapodymCohortFake* cohortPtr = new SeapodymCohortFake(nm, nd, taskIds[i]);
        cohortsPerWorker[i] = cohortPtr;
    }

    // Iterate over the global time steps
    for (auto istep = 0; istep < nt; ++istep) {

        // Iterate over the tasks assigned to this worker
        for (auto itask = 0; itask < taskIds.size(); ++itask) {
        
            // Get the task Id
            int taskId = taskIds[itask];

            // Get the number of steps for this task
            int numSteps = taskNumSteps[itask];

            std::cout <<   "Worker " << workerId << " processing task " << taskId 
                      << " at time step " << istep << " with " << numSteps - step_counter[itask] << " remaining steps." << std::endl;  
            
            // Simulate the time taken for a step
            cohortsPerWorker[itask]->stepForward(dvar_vector());
            
            // Done with the step
            step_counter[itask]++;

            // Find out whether we have to switch to another task
            if (step_counter[itask] >= numSteps) {
                // Switch over to a new task, reset the step counter and the number of steps
                taskIds[itask] = taskManager.getNextTask(taskId);
                step_counter[itask] = 0;
                taskNumSteps[itask] = taskManager.getNumSteps(taskIds[itask]);

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
