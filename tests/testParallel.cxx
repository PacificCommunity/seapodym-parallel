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

    SeapodymCohortFake cohort(100, 10, workerId); // Create a fake cohort for testing
    SeapodymCourier courier(MPI_COMM_WORLD);
    std::vector<double> data(nd, workerId); // Initialize data array with the processor rank
    courier.expose(data.data(), nd); // Expose the data to other workers

    // Initialize the SeapodymTaskManager
    SeapodymTaskManager taskManager(na, numWorkers, nt);

    // One worker may handle multiple tasks
    for (auto taskId : taskManager.getInitTaskIds(workerId)) {

        // Number of steps to run for this task
        int numSteps = taskManager.getNumSteps(taskId);
        std::cout << "Worker " << workerId << " has task " << taskId << " with " << numSteps << " steps." << std::endl;

        // march forward
        for (int step = 0; step < numSteps; ++step) {
            // Simulate a forward step
            cohort.stepForward(dvar_vector());
        }

        std::cout << "Worker " << workerId << " completed task " << taskId << "." << std::endl;
        // Get the dependencies of the new task on older tasks
        std::set<int> dependencies = taskManager.getDependencies(taskId);

        // Accumulate the data from all workers
        std::set<int> sourceWorkers;
        for (int i = 0; i < numWorkers; ++i) {
            if (i != workerId) {
                sourceWorkers.insert(i);
            }
        }
        courier.accumulate(sourceWorkers, workerId);

    }
    courier.free(); // Free the MPI window


    MPI_Finalize();
    return 0;
}
