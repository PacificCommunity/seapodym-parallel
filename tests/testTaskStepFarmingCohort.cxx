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
    cmdLine.set("-nt", 5, "Total number umber of steps");
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
    int numTasks = numAgeGroups + numTimeSteps - 1;

    // Workers expect a function that takes a single argument
    auto taskFunc1 = std::bind(taskFunc2, std::placeholders::_1, milliseconds);

    // set the number of steps for each task
    std::map<int, int> numStepsMap;
    for (int task_id = 0; task_id < numTasks; ++task_id) {
        numStepsMap[task_id] = std::min(
            std::min(numAgeGroups, task_id + 1),
            numTimeSteps + numAgeGroups - task_id - 1
        );
    }

    // infer the dependency taskId => {[taskId, step], ...}
    std::map<int, std::set<std::array<int, 2>>> dependencyMap;

    // no dependency for task_id 0 ... numAgeGroups - 1 but still need to 
    // have a key for those tasks
    for (int task_id = 0; task_id < numAgeGroups; ++task_id) {
        std::set< std::array<int, 2>> dep_set;
        dependencyMap[task_id] = dep_set;
    }
    for (int task_id = numAgeGroups; task_id < numTasks; ++task_id) {

        std::set< std::array<int, 2>> dep_set;
        int step;


        for (int i = 0; i < numAgeGroups; ++i) {

            int otherTaskId = task_id - i - 1;

            if (otherTaskId < numAgeGroups - 1) {
                // special
                step = task_id - numAgeGroups;
            } else {
                // regular
                step = i;
            }
            dep_set.insert(std::array<int, 2>{otherTaskId, step});
        }
    
        dependencyMap[task_id] = dep_set;

        // print the dependencies for debugging
        if (workerId == 0) {
            std::cout << "Task " << task_id << " has " << numStepsMap[task_id] << " steps and depends on ";
            for (auto d : dep_set) {
                std::cout << d[0] << ":" << d[1] << ", ";
            }
            std::cout << std::endl;
        }
    }

    if (workerId == 0) {

        // Manager
        
        std::cout << "Setting up the manager...\n";
        TaskStepManager manager(MPI_COMM_WORLD, numTasks, numStepsMap, dependencyMap);

        double tic = MPI_Wtime();

        // container stores the results TaskId, step, result
        std::cout << "Running the manager...\n";
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
        TaskStepWorker worker(MPI_COMM_WORLD, taskFunc1, numStepsMap);
        worker.run();

    }
    
    // Clean up
    MPI_Finalize();
    return 0;
}
