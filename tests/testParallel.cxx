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

    SeapodymCourier courier(MPI_COMM_WORLD);

    // Initialize the data array with the worker ID 
    std::vector<double> data(nd, workerId);
    // Buffer for communication
    std::vector<double> buffer(nd);

    // Expose the data to other workers
    courier.expose(data.data(), nd); 

    // Initialize the SeapodymTaskManager
    SeapodymTaskManager taskManager(na, numWorkers, nt);
    std::vector<int> taskIds = taskManager.getInitTaskIds(workerId);

    // Assign the tasks to the worker
    std::vector<int> workerTasks;
    for (auto taskId : taskIds) {
        if (taskId % numWorkers == workerId) {
            workerTasks.push_back(taskId);
        }
    }

    std::set<int> allWorkers;
    for (int i = 0; i < numWorkers; ++i) {
        allWorkers.insert(i);
    }

    int taskStep = 0;

    // Step in time
    for (int istep = 0; istep < nt; ++istep) {

        // Each worker processes its assigned tasks
        for (auto itask = 0; itask < workerTasks.size(); ++itask) {

            int taskId = workerTasks[itask];

            // Simulate processing the task
            std::cout << "Worker " << workerId << " processing task " << taskId << " at time step " << istep << "." << std::endl;

            if (taskStep >= taskManager.getNumSteps(taskId)) {
                // Switch over to a new task and reset the step counter
                taskStep = 0;
                workerTasks[itask] = taskManager.getNextTask(taskId);
            }
            else {
                // Keep on stepping forward the current task
                ++taskStep;
            }

        }

    }







    // // Iterate over the tasks assigned to this worker
    // for (auto taskId : workerTasks) {

    //     std::cout << "Worker " << workerId << " assigned task " << taskId << "." << std::endl;

    //     // Create a new cohort
    //     SeapodymCohortFake* cohort = new SeapodymCohortFake(nm, nd, taskId);

    //     // Step forward
    //     for (int step = 0; step < taskManager.getNumSteps(taskId); ++step) {
    //         // Simulate a forward step
    //         cohort->stepForward(dvar_vector());
    //     }

    //     // The cohort reached its end of life
    //     std::cout << "Worker " << workerId << " completed task " << taskId << "." << std::endl;
    //     delete cohort; // Delete the cohort after processing

    //     // Get the next task
    //     int nextTaskId = taskManager.getNextTaskId(taskId);
        
    // }




    // // One worker may handle multiple tasks (or cohorts)
    // for (auto taskId : taskManager.getInitTaskIds(workerId)) {

    //     // Create a new task for the worker
    //     SeapodymCohortFake* cohort = new SeapodymCohortFake(nm, nd, taskId);


    //     // Number of steps to run for this task
    //     int numSteps = taskManager.getNumSteps(taskId);
    //     std::cout << "Worker " << workerId << " has task " << taskId << " with " << numSteps << " steps." << std::endl;

    //     // March forward
    //     for (int step = 0; step < numSteps; ++step) {
    //         // Simulate a forward step
    //         cohort->stepForward(dvar_vector());
    //     }

    //     std::cout << "Worker " << workerId << " completed task " << taskId << "." << std::endl;
    //     delete cohort; // Delete the cohort after processing

    //     // Get the next task
    //     int 

    //     cohort = new SeapodymCohortFake(nm, nd, workerId); // Create a new cohort for the next task



    //     // Get the dependencies of the new task on older tasks
    //     std::set<int> dependencies = taskManager.getDependencies(taskId);

    //     // // Accumulate the data from all workers
    //     // std::set<int> sourceWorkers;
    //     // for (int i = 0; i < numWorkers; ++i) {
    //     //     if (i != workerId) {
    //     //         sourceWorkers.insert(i);
    //     //     }
    //     // }
    //     // courier.accumulate(sourceWorkers, workerId);

    // }

    // Gather the results from all workers TO DO 

    courier.free(); // Free the MPI window


    MPI_Finalize();
    return 0;
}
