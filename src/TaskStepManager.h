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

        // dependencies
        std::map<int, std::set<dep_type> > deps;

    public:

        /**
         * Constructor
         * @param comm communicator
         * @param numTasks number of tasks
         */
        TaskStepManager(MPI_Comm comm, int numTasks);

        /**
         * Add dependencies
         * @param taskId Id of the task
         * @param otherTaskIds Dependencies of the above task { [taskId, step],  ...}
         */
        void addDependencies(int taskId, const std::set< dep_type >& otherTaskIds);

        /**
         * Run the manager
         * @return (taskId, step, result) tuples for each task
         */
        std::set< std::array<int, 3> > run() const;

};

#endif // TASK_DEPENDENCY_MANAGER
