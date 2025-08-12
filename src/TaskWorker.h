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

        // number of age groups for each time step, ideally matching the number of workers
        MPI_Comm comm; 

        // number of workers 0...numWorkers-1
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
