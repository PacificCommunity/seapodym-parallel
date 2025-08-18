#include <mpi.h>
#include <iostream>
#include <functional>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <CmdLineArgParser.h>
#include "TaskStepManager.h"
#include "TaskStepWorker.h"

// Task for the workers to execute
/**
 * Task
 * @param task_id index 0.. numTasks - 1
 * @param ms Sleep # milliseconds
 * @return result
 */
int taskFunc2(int task_id, int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    return task_id;
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
    cmdLine.set("-na", 5, "Number of age groups");
    cmdLine.set("-nt", 5, "Total number of time steps");
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

    int numAgeGroups = cmdLine.get<int>("-na");
    int numTimeSteps = cmdLine.get<int>("-nt");
    int milliseconds = cmdLine.get<int>("-nm");
    int numTasks = numAgeGroups * int( std::ceil( float(numTimeSteps) / float(numAgeGroups) ) );

    // Workers expect a function that takes a single argument
    auto taskFunc1 = std::bind(taskFunc2, std::placeholders::_1, milliseconds);

    // Infer the dependency taskId => {[taskId, step], ...}

    if (workerId == 0) {

        // Manager
        
        TaskStepManager manager(MPI_COMM_WORLD, numTasks, numTimeSteps);

        // build the dependency table
        for (int taskId = 0; taskId < numTasks; ++taskId) {
            std::set<dep_type> deps;
            for (int tid = std::max(0, taskId - numAgeGroups); tid < taskId; ++tid) {
                deps.insert( std::array{tid, taskId - tid - 1});
            }
            manager.addDependencies(taskId, deps);

            std::cout << "Task " << taskId << " depends on ";
            for (auto d : deps) {
                std::cout << d[0] << ":" << d[1] << ", ";
            }
            std::cout << std::endl;
        }

        double tic = MPI_Wtime();

        // container stores the results TaskId, step, result
        std::set< std::array<int, 3> > results = manager.run();

        double toc = MPI_Wtime();

        std::cout << "Execution time: " << toc - tic << 
            " Speedup: " << 0.001*double(numTasks * milliseconds)/(toc - tic) << 
            " Ideal: " << numWorkers << std::endl;

        for (auto [taskId, step, res] : results) {
            std::cout << "task " << taskId << "@step " << step << " => " << res << std::endl;
        }

        // Make sure there are no duplicate tasks and all the tasks have been eceuted
        assert(results.size() == numTasks);
        for (auto [taskId, step, res] : results) {
            assert(taskId == res);
        }

        std::cout << "Success\n";

    } else {

        // Worker
        TaskStepWorker worker(MPI_COMM_WORLD, taskFunc1);
        
        // most cohorts will run for numAgeGroups steps, except at the beginning and at the end
        worker.run(numAgeGroups);

    }
    

    // Clean up
    MPI_Finalize();
    return 0;
}
