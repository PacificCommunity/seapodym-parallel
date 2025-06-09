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
    int na = cmdLine.get<int>("-na");
    int nt = cmdLine.get<int>("-nt");
    int nm = cmdLine.get<int>("-nm");
    int nd = cmdLine.get<int>("-nd");
    if (na <= 0 || nt <= 0 || nm <= 0 || nd <= 0) {
        if (workerId == 0) {
            // Print an error message only from the master worker
            std::cerr << "Invalid command line arguments. All values must be positive integers." << std::endl;
        }
        cmdLine.help();
        MPI_Finalize();
        return 1;
    }
    if (na <= 0 || nt <= 0 || na < numWorkers) {
        if (workerId == 0) {
            // Print an error message only from the master worker
            std::cerr << "Invalid number of age groups (" << na << "), time steps (" << nt << ") or workers (" << numWorkers << ").\n";
            std::cerr << "Number of age groups must be positive and at least equal to the number of workers." << std::endl;
            cmdLine.help();
        }
        MPI_Finalize();
        return 2;
    }

    if (workerId == 0) {
        std::cout << "Running with " << numWorkers << " workers." << std::endl;
        std::cout << "Number of age groups: " << na << ", Total number of time steps: " << nt << std::endl;
    }


    SeapodymTaskManager taskManager(na, numWorkers, nt);
    std::vector<int> taskIds = taskManager.getInitTaskIds(workerId);
    SeapodymCourier courier(MPI_COMM_WORLD);

    std::vector<double> data(nd, workerId);
    courier.expose(data.data(), nd);

    std::vector<int> initTaskIds = taskManager.getInitTaskIds(workerId);
    for (auto itask = 0; itask < taskIds.size(); ++itask) {
        int taskId = initTaskIds[itask];
        int step_counter = 0;
        int numSteps = taskManager.getNumSteps(taskId);
        for (auto istep = 0; istep < nt; ++istep) {
            std::cout <<   "Worker " << workerId << " processing task " << taskId 
                      << " at time step " << istep << " with " << numSteps << " steps." << std::endl;  
            step_counter++;
            if (step_counter >= numSteps) {
                // Switch over to a new task and reset the step counter
                step_counter = 0;
                taskId = taskManager.getNextTask(taskId);
                numSteps = taskManager.getNumSteps(taskId);
                // fetch the data from the other workers
                courier.fetch(0); //MPI_ANY_SOURCE); // Fetch data from any source worker
                double* fetchedData = courier.getDataPtr();
            }
        }
    }


    courier.free(); // Free the MPI window


    MPI_Finalize();
    return 0;
}
