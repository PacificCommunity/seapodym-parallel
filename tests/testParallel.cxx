#include <SeapodymCohortFake.h>
#include <SeapodymTaskManager.h>
#include <SeapodymCourier.h>
#include <CmdLineArgParser.h>
#include <mpi.h>
#include <admodel.h>
#include <iostream>

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);
    
    CmdLineArgParser cmdLine;
    cmdLine.set("-na", 3, "Number of age groups");
    cmdLine.set("-nt", 5, "Total number of time steps");
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
    int na = cmdLine.get<int>("-na");
    int nt = cmdLine.get<int>("-nt");
    int numWorkers;
    MPI_Comm_size(MPI_COMM_WORLD, &numWorkers);
    int workerId;
    MPI_Comm_rank(MPI_COMM_WORLD, &workerId);
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

    SeapodymCohortFake cohort(100, 10, workerId); // Create a fake cohort for testing
    SeapodymCourier courier(MPI_COMM_WORLD);

    // Initialize the SeapodymTaskManager
    SeapodymTaskManager taskManager(na, numWorkers, nt);

    for (auto taksId : taskManager.getInitTaskIds(workerId)) {
        int numSteps = taskManager.getNumSteps(taksId);
        std::cout << "Worker " << workerId << " has task " << taksId << " with " << numSteps << " steps." << std::endl;

        // march forward
        for (int step = 0; step < numSteps; ++step) {
            // Simulate a forward step
            cohort.stepForward(dvar_vector());
        }
        std::cout << "Worker " << workerId << " completed task " << taksId << "." << std::endl;
        // Get the dependencies of the new task on older tasks
        std::set<int> dependencies = taskManager.getDependencies(taksId);
        // TO DO.... accumulate the data from all workers

    }


    MPI_Finalize();
    return 0;
}
