#include <mpi.h>

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
        int (*taskFunc)(int);

    public:

        /**
         * Constructor
         * @param comm MPI communicator
         * @param taskFunc task function
         * @return success (0) or failure
         */
        TaskWorker(MPI_Comm comm, int (*taskFunc)(int));

        /**
         * Run the tasks assigned by the TaskManager
         * @param workerId worker ID
         * @return list
         */
        void run();

};

#endif // TASK_WORKER
