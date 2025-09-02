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
#include "DistDataCollector.h"

// Task for the workers to execute
/**
 * Task
 * @param task_id index 0.. numTasks - 1
 * @param stepBeg first step index (inclusive)
 * @param stepEnd last step index (exclusive)
 * @param comm MPI communicator
 * @param ms Sleep # milliseconds
 */
void taskFunction(int task_id, int stepBeg, int stepEnd, MPI_Comm comm, 
    int ms, int numAgeGroups, int numData, DistDataCollector* dataCollector) {

    std::vector<double> localData(numData);

    // step through...
    for (auto step = stepBeg; step < stepEnd; ++step) {

        // Perform the work, just sleeping here zzzzzzz
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));

        // Pretend we are computing some data
        std::fill(localData.begin(), localData.end(), double(task_id));
        
        // Send the data to the manager. Here, the data are 
        // collected row by row. The entry into the collected 
        // array is at index chunk_id.
        int row = task_id - numAgeGroups + 1 + step;
        int col = task_id % numAgeGroups;
        int chunk_id = row*numAgeGroups + col;
        dataCollector->put(chunk_id, localData.data());

        // E.g.
        int success = task_id;

        // Notify the manager at the end of each step
        int output[3] = {task_id, step, success};
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
    cmdLine.set("-na", 5, "Number of age groups");
    cmdLine.set("-nt", 5, "Total number of steps");
    cmdLine.set("-nm", 100, "Sleep milliseconds");
    cmdLine.set("-nd", 10000, "Number of data values to send from worker to manager at each step");
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
    int numData = cmdLine.get<int>("-nd");

    // analyze the cohort Id task dependencies
    SeapodymCohortDependencyAnalyzer taskDeps(numAgeGroups, numTimeSteps);
    int numCohorts = taskDeps.getNumberOfCohorts();
    int numCohortSteps = taskDeps.getNumberOfCohortSteps();
    std::map<int, int> stepBegMap = taskDeps.getStepBegMap();
    std::map<int, int> stepEndMap = taskDeps.getStepEndMap();
    std::map<int, std::set<std::array<int, 2>>> dependencyMap = taskDeps.getDependencyMap();

    // print the dependencies for debugging
    if (workerId == 0) {
        for (const auto& [task_id, stepBeg] : stepBegMap) {
            int globalTimeIndex = std::max(0, task_id - numAgeGroups + 1);
            std::cout << "At time " << globalTimeIndex << " Task " << task_id << " has steps " << stepBeg << "..." << stepEndMap.at(task_id) - 1 << " and depends on: ";
            for (const auto& [task_id2, step] : dependencyMap.at(task_id)) {
                std::cout << task_id2 << ":" << step << ", ";
            }
            std::cout << std::endl;
        }
    }

    // set up the data collector
    int numChunks = numAgeGroups * numTimeSteps;
    DistDataCollector dataCollect(MPI_COMM_WORLD, numChunks, numData);

    // workers expect a function that takes a 4 arguments. We bind the last
    // argument to milliseconds
    auto taskFunc = std::bind(taskFunction, 
        std::placeholders::_1, // task_id
        std::placeholders::_2, // stepBeg
        std::placeholders::_3, // stepEnd
        std::placeholders::_4, // comm
        milliseconds,
        numAgeGroups,
        numData,
        &dataCollect);

    if (workerId == 0) {

        // Manager
        
        // note: the number of tasks is the number of cohorts
        TaskStepManager manager(MPI_COMM_WORLD, numCohorts, stepBegMap, stepEndMap, dependencyMap);

        double tic = MPI_Wtime();

        // container stores the results TaskId, step, result
        const auto results = manager.run();

        double toc = MPI_Wtime();

        auto numTotalSteps = results.size();
        std::cout << "Execution time: " << toc - tic << 
            " Speedup: " << 0.001*double(numTotalSteps * milliseconds)/(toc - tic) << 
            " Ideal: " << numWorkers << std::endl;


        // make sure there are no duplicate tasks and all the tasks have been executed
        assert(numTotalSteps == numCohortSteps);
        for (auto [taskId, step, res] : results) {
            // in this test we return the task_id when we're finished
            assert(taskId == res);
        }

        std::cout << "Success\n";

    } else {

        // Worker
        TaskStepWorker worker(MPI_COMM_WORLD, taskFunc, stepBegMap, stepEndMap);
        worker.run();

    }

    dataCollect.free();
    
    // Clean up
    MPI_Finalize();
    return 0;
}
