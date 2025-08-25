#include <mpi.h>
#include <map>

#ifndef TASK_MANAGER
#define TASK_MANAGER

/**
 * Class TaskManager
 * @brief The TaskManager assigns tasks to TaskWorkers. In this simple version of task farming, there are no dependencies 
 *        between tasks. Use this in conjunction with TaskWorker when the tasks can be performed in any order. 
 * @see TaskWorker
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
        std::map<int, int> run() const;

};

#endif // TASK_MANAGER
