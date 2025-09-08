#include "TaskStepWorker.h"
#include "Tags.h"
#include <array>

TaskStepWorker::TaskStepWorker(MPI_Comm comm, 
  std::function<void(int, int, int, MPI_Comm)> taskFunc,
  const std::map<int, int>& stepBegMap, 
  const std::map<int, int>& stepEndMap) {
    this->comm = comm;
    this->taskFunc = taskFunc;
    this->stepBegMap = stepBegMap;
    this->stepEndMap = stepEndMap;
}
        
void
TaskStepWorker::run() const {

    const int manager_rank = 0;
    int workerId;
    MPI_Comm_rank(this->comm, &workerId);

    const int managerRank = 0;

    while (true) {

        // Get the task_id to operate on
        int task_id;
        MPI_Recv(&task_id, 1, MPI_INT, managerRank, START_TASK_TAG, this->comm, MPI_STATUS_IGNORE);

        if (task_id == -1) {
            // Shutdown
            break;
        }

        int stepBeg = this->stepBegMap.at(task_id);
        int stepEnd = this->stepEndMap.at(task_id);

        // Perform the task, which includes stepping from stepBeg to stepEnd - 1.
        // This function should notify the manager at the end of each step
        this->taskFunc(task_id, stepBeg, stepEnd, this->comm);

        // Notify the manager that this worker is available again
        int done = 1;
        MPI_Send(&done, 1, MPI_INT, managerRank, WORKER_AVAILABLE_TAG, MPI_COMM_WORLD);
    }
 
}
