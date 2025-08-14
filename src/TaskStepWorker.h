#include <mpi.h>
#include <functional>

#ifndef TASK_STEP_WORKER
#define TASK_STEP_WORKER

/**
 * Class TaskStepWorker
 * @brief The TaskStepWorker gets tasks assigned and executes them.
 */

class TaskStepWorker {

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
        TaskStepWorker(MPI_Comm comm, std::function<int(int)> taskFunc);

        /**
         * Run the tasks assigned by the TaskManager
         * @param numSteps number steps
         */
        void run(int numSteps) const;

};

#endif // TASK_STEP_WORKER
