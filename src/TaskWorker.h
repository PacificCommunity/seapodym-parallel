#include <mpi.h>
#include <functional>

#ifndef TASK_WORKER
#define TASK_WORKER

/**
 * Class TaskWorker
 * @brief The TaskWorker gets tasks assigned and executes them.
 */

class TaskWorker {

    private:

        // Communicator
        MPI_Comm comm; 

        // Task function
        std::function<int(int)> taskFunc;

    public:

        /**
         * Constructor
         * @param comm MPI communicator
         * @param taskFunc task function
         */
        TaskWorker(MPI_Comm comm, std::function<int(int)> taskFunc);

        /**
         * Run the tasks assigned by the TaskManager
         * @param workerId worker ID
         */
        void run() const;

};

#endif // TASK_WORKER
