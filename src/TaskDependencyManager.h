#include <mpi.h>
#include <vector>
#include <map>

#ifndef TASK_DEPENDENCY_MANAGER
#define TASK_DEPENDENCY_MANAGER

/**
 * Class TaskDependencyManager
 * @brief The TaskDependencyManager assigns tasks with dependencies to TaskDependencyWorkers.
 */

class TaskDependencyManager {

    private:

        // Communicator
        MPI_Comm comm; 

        // number of tasks
        int numTasks;

        // dependencies
        std::map<int, std::vector<int> > deps;

    public:

        /**
         * Constructor
         * @param comm communicator
         * @param numTasks number of tasks
         */
        TaskDependencyManager(MPI_Comm comm, int numTasks);

        /**
         * Add a dependency 
         * @param taskId Id of the dependent task
         * @param otherTaskIds Dependencies
         */
        void addDependency(int taskId, const std::vector<int>& otherTaskIds);

        /**
         * Run the manager
         * @return the result of each task
         */
        std::vector<int> run() const;

};

#endif // TASK_DEPENDENCY_MANAGER
