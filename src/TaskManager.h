#include <mpi.h>
#include <vector>

#ifndef TASK_MANAGER
#define TASK_MANAGER

/**
 * Class TaskManager
 * @brief The TaskManager assigns tasks to TaskWorkers.
 */

class TaskManager {

    private:

        // Communicator
        MPI_Comm comm; 

        // number of tasks
        int numTasks;

    public:

        /**
         * Constructor
         * @param comm communicator
         * @param numTasks number of tasks
         */
        TaskManager(MPI_Comm comm, int numTasks);

        /**
         * Run the manager
         * @return results of each task
         */
        std::vector<int> run() const;

};

#endif // TASK_MANAGER
