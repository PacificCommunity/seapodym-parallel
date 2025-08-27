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
 * @param stepBeg first step index (inclusive)
 * @param stepEnd last step index (exclusive)
 * @param comm MPI communicator
 * @param ms Sleep # milliseconds
 */
void taskFunc2(int task_id, int stepBeg, int stepEnd, MPI_Comm comm, int ms) {

    for (auto i = stepBeg; i < stepEnd; ++i) {
        // Perform the work
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));

        // Notify the manager at the end of each step
        int output[3] = {task_id, i, task_id};
        const int endTaskTag = 1;
        MPI_Send(output, 3, MPI_INT, 0, endTaskTag, comm);
    }
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
    cmdLine.set("-nT", 5, "Number of tasks/cohorts");
    cmdLine.set("-ns", 5, "Number of steps for each task");
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
    int numSteps = cmdLine.get<int>("-ns");
    int milliseconds = cmdLine.get<int>("-nm");

    // Workers expect a function that takes a single argument
    auto taskFunc1 = std::bind(taskFunc2, 
        std::placeholders::_1, // task_id
        std::placeholders::_2, // stepBeg
        std::placeholders::_3, // stepEnd
        std::placeholders::_4, // comm
        milliseconds);

    // set the number of steps for each task
    std::map<int, int> stepBegMap;
    std::map<int, int> stepEndMap;
    for (int task_id = 0; task_id < numTasks; ++task_id) {
        // in this version it is the same for each task
        stepBegMap[task_id] = 0;
        stepEndMap[task_id] = numSteps;
    }

    // infer the dependency taskId => {[taskId, step], ...}
    std::map<int, std::set<std::array<int, 2>>> dependencyMap;
    for (int task_id = 0; task_id < numTasks; ++task_id) {
        std::set< std::array<int, 2>> dep_set;
        for (int i = 0; i < numSteps; ++i) {
            if (task_id - i - 1 >= 0) {
                dep_set.insert(std::array<int, 2>{task_id - i - 1, i});
            }
        }
        dependencyMap[task_id] = dep_set;
        // print the dependencies for debugging
        if(workerId == 0) {
            std::cout << "Task " << task_id << " has steps " << stepBegMap[task_id] << "..." <<  stepEndMap[task_id] - 1 
                << " and depends on ";
            for (auto d : dep_set) {
                std::cout << d[0] << ":" << d[1] << ", "; 
            }
            std::cout << std::endl;
        }
    }

    if (workerId == 0) {

        // Manager
        
        TaskStepManager manager(MPI_COMM_WORLD, numTasks, stepBegMap, stepEndMap, dependencyMap);

        double tic = MPI_Wtime();

        // container stores the results (taskId, step, result)
        auto results = manager.run();

        double toc = MPI_Wtime();

        int numTotalSteps = 0;
        for (const auto& [taskId, step, res] : results) {
            numTotalSteps++;
        }

        std::cout << "Execution time: " << toc - tic << 
            " Speedup: " << 0.001*double(numTotalSteps * milliseconds)/(toc - tic) << 
            " Ideal: " << numWorkers << std::endl;

        // for (auto [taskId, step, res] : results) {
        //     std::cout << "task " << taskId << "@step " << step << " result: " << res << std::endl;
        // }

        // make sure there are no duplicate tasks and all the tasks have been executed
        assert(results.size() == numTasks);
        for (auto [taskId, step, res] : results) {
            assert(taskId == res);
        }

        std::cout << "Success\n";

    } else {

        // Worker
        TaskStepWorker worker(MPI_COMM_WORLD, taskFunc1, stepBegMap, stepEndMap);
        worker.run();

    }
    
    // Clean up
    MPI_Finalize();
    return 0;
}
