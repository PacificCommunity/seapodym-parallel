#include <mpi.h>
#include <functional>
#include <map>

#ifndef TASK_STEP_WORKER
#define TASK_STEP_WORKER

/**
 * Class TaskStepWorker
 * @brief The TaskStepWorker gets tasks assigned from the TaskStepManager and executes them.
 * 
 * @details A task involves running multiple steps and the worker will inform the manager once 
 *          once various steps have been accomplished. 
 * 
 * @see TaskStepManager
 */

class TaskStepWorker {

    private:

        // communicator
        MPI_Comm comm; 

        // task function
        std::function<int(int)> taskFunc;

        // task to number of steps map
        std::map<int, int> numStepsMap;

    public:

        /**
         * Constructor
         * @param comm MPI communicator
         * @param taskFunc task function
         * @param numStepsMap map of task to number of steps
         */
        TaskStepWorker(MPI_Comm comm, std::function<int(int)> taskFunc, std::map<int, int> numStepsMap);

        /**
         * Run the tasks assigned by the TaskManager
         * @param numSteps number steps
         */
        void run() const;

};

#endif // TASK_STEP_WORKER
