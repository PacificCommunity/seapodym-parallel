#include <mpi.h>
#include <map>
#include <set>
#include <array>

#ifndef TASK_DEPENDENCY_MANAGER
#define TASK_DEPENDENCY_MANAGER

/**
 * Class TaskStepManager
 * @brief The TaskStepManager assigns tasks with dependencies to TaskDependencyWorkers.
 */

// TaskId 
using dep_type = std::array<int, 2>;


class TaskStepManager {

    private:

        // Communicator
        MPI_Comm comm; 

        // number of tasks
        int numTasks;

        // number of steps
        int numSteps;

        // dependencies
        std::map<int, std::set<dep_type> > deps;

    public:

        /**
         * Constructor
         * @param comm communicator
         * @param numTasks number of tasks
         * @param numSteps number of steps for each task
         * @param dependencyMap map of task dependencies {taskId: {taskId, step}, ...}
         */
        TaskStepManager(MPI_Comm comm, int numTasks, int numSteps, 
            std::map<int, std::set<dep_type> > dependencyMap);

        /**
         * Run the manager
         * @return (taskId, step, result) tuples for each task
         */
        std::set< std::array<int, 3> > run() const;

};

#endif // TASK_DEPENDENCY_MANAGER
