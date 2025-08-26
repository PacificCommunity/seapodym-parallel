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
        std::function<int(int, int, int, int)> taskFunc;

        // task Id to first step index map
        std::map<int, int> stepBegMap;

        // task Id to last step index + 1 map
        std::map<int, int> stepEndMap;

    public:

        /**
         * Constructor
         * @param comm MPI communicator
         * @param taskFunc task function
         * @param stepBegMap map of task Id to first step index
         * @param stepEndMap map of task Id to last step index + 1
         */
        TaskStepWorker(MPI_Comm comm, std::function<int(int, int, int, int)> taskFunc, 
            std::map<int, int> stepBegMap, std::map<int, int> stepEndMap);

        /**
         * Run the tasks assigned by the TaskManager
         */
        void run() const;

};

#endif // TASK_STEP_WORKER
