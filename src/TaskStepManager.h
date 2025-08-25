#include <mpi.h>
#include <map>
#include <set>
#include <array>

#ifndef TASK_DEPENDENCY_MANAGER
#define TASK_DEPENDENCY_MANAGER

/**
 * Class TaskStepManager
 * @brief The TaskStepManager assigns tasks with (task, step) dependencies to TaskStepWorkers.
 * 
 * @details This manager should be used when each task involves multiple steps and task
 *          depend not only on other tasks, but on particular steps performed by the 
 *          tasks. The manager must know how many steps to execute for each task. After each step, 
 *          workers will inform the manager that the step has been executed and the manager
 *          will assign the next step to the worker if it is available. The manager will also
 *          ensure that the dependencies are satisfied before assigning a task to a worker.
 * 
 * @see TaskStepWorker
 */

// TaskId 
using dep_type = std::array<int, 2>;

/**
 * @brief The TaskStepManager orchestrates tasks that depend on other tasks' steps. Use 
 *        this in conjunction with TaskStepWorker.
 * 
 * @see TaskStepWorker
 */
class TaskStepManager {

    private:

        // Communicator
        MPI_Comm comm; 

        // number of tasks
        int numTasks;

        // taskId to first step index map
        std::map<int, int> stepBegMap;

        // taskId to last step index + 1 map
        std::map<int, int> stepEndMap;

        // dependencies
        std::map<int, std::set<dep_type> > deps;

    public:

        /**
         * Constructor
         * @param comm communicator
         * @param numTasks number of tasks
         * @param stepBegMap taskId -> first step map
         * @param stepEndMap taskId -> last step + 1 map
         * @param dependencyMap map of task dependencies {taskId: {taskId, step}, ...}
         */
        TaskStepManager(MPI_Comm comm, int numTasks, 
            const std::map<int, int>& stepBegMap,
            const std::map<int, int>& stepEndMap,
            const std::map<int, std::set<dep_type> >& dependencyMap);

        /**
         * Run the manager
         * @return (taskId, step, result) tuples for each task
         */
        std::set< std::array<int, 3> > run() const;

};

#endif // TASK_DEPENDENCY_MANAGER
