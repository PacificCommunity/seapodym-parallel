#include <mpi.h>
#include <map>
#include <set>

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
        std::map<int, std::set<int> > deps;

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
        void addDependencies(int taskId, const std::set<int>& otherTaskIds);

        /**
         * Run the manager
         * @return the result of each task
         */
        std::map<int, int> run() const;

};

#endif // TASK_DEPENDENCY_MANAGER
