#include <mpi.h>
#include <functional>
#include <map>

#ifndef TASK_STEP_WORKER
#define TASK_STEP_WORKER

/**
 * Class TaskStepWorker
 * @brief The TaskStepWorker gets tasks assigned from the TaskStepManager and executes them.
 * 
 * @details A task involves running multiple steps and the worker will inform the manager after each step is complete.
 * 
 * @see TaskStepManager
 */

class TaskStepWorker {

    private:

        // communicator
        MPI_Comm comm; 

        // task function, takes task_id, stepBeg and stepEnd as input 
        // and returns a code/result
        std::function<void(int, int, int, MPI_Comm)> taskFunc;

        // task Id to first step index map
        std::map<int, int> stepBegMap;

        // task Id to last step index + 1 map
        std::map<int, int> stepEndMap;

    public:

        /**
         * Constructor
         * @param comm MPI communicator
         * @param taskFunc task function takes task_id, stepBeg, stepEnd and the MPI communicator
         *                 as input arguments. This function should include a call 
         *                 notifying the manager at the end of each step, ie 
         *                 MPI_Send({task_id, step, result}, 3, MPI_INT, managerRank, END_TASK_TAG, comm);
         * @param stepBegMap map of task Id to first step index
         * @param stepEndMap map of task Id to last step index + 1
         */
        TaskStepWorker(MPI_Comm comm, 
            std::function<void(int, int, int, MPI_Comm)> taskFunc, 
            const std::map<int, int>& stepBegMap,
            const std::map<int, int>& stepEndMap);

        /**
         * Run the tasks assigned by the TaskManager
         */
        void run() const;

};

#endif // TASK_STEP_WORKER
