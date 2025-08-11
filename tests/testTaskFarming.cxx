#include <mpi.h>
#include <iostream>
#include <CmdLineArgParser.h>
#include "TaskManager.h"
#include "TaskWorker.h"

// Some function for the workers to execute
int taskFunc(int task_id) {
    return task_id * task_id;
}

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);
    int numWorkers, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    numWorkers = size - 1;
    int workerId;
    MPI_Comm_rank(MPI_COMM_WORLD, &workerId);
    
    CmdLineArgParser cmdLine;
    cmdLine.set("-nT", 5, "Total number of tasks");
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

    if (workerId == 0) {
        // Manager
        TaskManager manager(MPI_COMM_WORLD, numTasks);
        std::vector<int> results = manager.run();
        std::cout << "Success\n";
    } else {
        // Worker
        TaskWorker worker(MPI_COMM_WORLD, taskFunc);
        worker.run();
    }
    

    MPI_Finalize();
    return 0;
}
