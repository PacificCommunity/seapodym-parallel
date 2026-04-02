#include <mpi.h>
#include <unordered_map>
#include <vector>

#ifndef TASK_DEPENDENCY_MANAGER
#define TASK_DEPENDENCY_MANAGER

/**
 * Class TaskDependencyManager
 * @brief The TaskDependencyManager assigns tasks with dependencies to TaskDependencyWorkers. The dependencies are arbitrary but do not
 *        involve any internal steps. Use this in conjunction with TaskWorker when the tasks have dependencies.
 * @see TaskWorker
 */

class TaskDependencyManager {

    private:

        // Communicator
        MPI_Comm comm; 

        // number of tasks
        int numTasks;

        // dependencies
        std::unordered_map<int, std::vector<int> > deps;

    public:

        /**
         * Constructor
         * @param comm communicator
         * @param numTasks number of tasks
         */
        TaskDependencyManager(MPI_Comm comm, int numTasks);

        /**
         * Add dependencies
         * @param taskId Id of the dependent task
         * @param otherTaskIds Dependencies
         */
        void addDependencies(int taskId, const std::vector<int>& otherTaskIds);

        /**
         * Run the manager
         * @return the result of each task
         */
        std::unordered_map<int, int> run() const;

};

#endif // TASK_DEPENDENCY_MANAGER
