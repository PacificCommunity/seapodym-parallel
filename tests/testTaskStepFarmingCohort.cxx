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
#include "SeapodymCohortDependencyAnalyzer.h"

// Task for the workers to execute
/**
 * Task
 * @param task_id index 0.. numTasks - 1
 * @param ms Sleep # milliseconds
 * @return result (could be an error flag)
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
    cmdLine.set("-nt", 5, "Total number number of steps");
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

    // workers expect a function that takes a single argument
    auto taskFunc1 = std::bind(taskFunc2, std::placeholders::_1, milliseconds);

    // analyze the conhort Id task dependencies
    SeapodymCohortDependencyAnalyzer taskDeps(numAgeGroups, numTimeSteps);
    int numCohorts = taskDeps.getNumberOfCohorts();
    int numCohortSteps = taskDeps.getNumberOfCohortSteps();
    std::map<int, int> numStepsMap = taskDeps.getNumStepsMap();
    std::map<int, std::set<std::array<int, 2>>> dependencyMap = taskDeps.getDependencyMap();

    // print the dependencies for debugging
    if (workerId == 0) {
        for (const auto& [task_id, nstep] : numStepsMap) {
            std::cout << "Task " << task_id << " has " << nstep << " steps and depends on: ";
            for (const auto& [task_id2, step] : dependencyMap.at(task_id)) {
                std::cout << task_id2 << ":" << step << ", ";
            }
            std::cout << std::endl;
        }
    }
    

    if (workerId == 0) {

        // Manager
        
        // note: the number of tasks is the number of cohorts, each task involves multiple steps
        TaskStepManager manager(MPI_COMM_WORLD, numCohorts, numStepsMap, dependencyMap);

        double tic = MPI_Wtime();

        // container stores the results TaskId, step, result
        const auto results = manager.run();

        double toc = MPI_Wtime();

        int numTotalSteps = 0;
        for (const auto& [taskId, step, res] : results) {
            numTotalSteps++;
        }

        std::cout << "Execution time: " << toc - tic << 
            " Speedup: " << 0.001*double(numTotalSteps * milliseconds)/(toc - tic) << 
            " Ideal: " << numWorkers << std::endl;


        // make sure there are no duplicate tasks and all the tasks have been executed
        assert(results.size() == numCohortSteps);
        for (auto [taskId, step, res] : results) {
            // in this test we return the task_id when we're finished
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
