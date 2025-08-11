#include <mpi.h>
#include <iostream>
#include <functional>
#include <thread>
#include <chrono>
#include <CmdLineArgParser.h>
#include "TaskManager.h"
#include "TaskWorker.h"

// Task for the workers to execute
/**
 * Task
 * @param task_id index 0.. numTasks - 1
 * @param ms Sleep # milliseconds
 * @return result
 */
int taskFunc2(int task_id, int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    return task_id * task_id;
}

int main(int argc, char** argv) {

    // MPI initialization
    MPI_Init(&argc, &argv);
    int numWorkers, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    numWorkers = size - 1;
    int workerId;
    MPI_Comm_rank(MPI_COMM_WORLD, &workerId);
    
    // Parse the command line arguments
    CmdLineArgParser cmdLine;
    cmdLine.set("-nT", 5, "Total number of tasks");
    cmdLine.set("-nm", 100, "Sleep milliseconds");
    bool success = cmdLine.parse(argc, argv);
    bool help = cmdLine.get<bool>("-help") || cmdLine.get<bool>("-h");
    if (!success) {
        std::cerr << "Error parsing command line arguments." << std::endl;
        cmdLine.help();
        MPI_Finalize();
        return 1;
    }
    if (help) {
        cmdLine.help();
        MPI_Finalize();
        return 1;
    }

    int numTasks = cmdLine.get<int>("-nT");
    int milliseconds = cmdLine.get<int>("-nm");

    // Workers expect a function that takes a single argument
    auto taskFunc1 = std::bind(taskFunc2, std::placeholders::_1, milliseconds);

    if (workerId == 0) {

        double tic = MPI_Wtime();

        // Manager
        TaskManager manager(MPI_COMM_WORLD, numTasks);
        std::vector<int> results = manager.run();

        double toc = MPI_Wtime();
        std::cout << "Execution time: " << toc - tic << 
            " Speedup: " << 0.001*double(numTasks * milliseconds)/(toc - tic) << 
            " Ideal: " << numWorkers << std::endl;
        std::cout << "Success\n";

    } else {

        // Worker
        TaskWorker worker(MPI_COMM_WORLD, taskFunc1);
        worker.run();

    }
    

    // Clean up
    MPI_Finalize();
    return 0;
}
