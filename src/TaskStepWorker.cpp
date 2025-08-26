#include "TaskStepWorker.h"
#include <array>

TaskStepWorker::TaskStepWorker(MPI_Comm comm, std::function<int(int)> taskFunc,
  std::map<int, int> stepBegMap, std::map<int, int> stepEndMap) {
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

    const int startTaskTag = 0;
    const int endTaskTag = 1;
    const int workerAvailableTag = 2;
    const int managerRank = 0;

    while (true) {

        // Get the task_id to operate on
        int task_id;
        MPI_Recv(&task_id, 1, MPI_INT, managerRank, startTaskTag, this->comm, MPI_STATUS_IGNORE);

        if (task_id == -1) {
            // Shutdown
            break;
        }

        int stepBeg = this->stepBegMap.at(task_id);
        int stepEnd = this->stepEndMap.at(task_id);

        // task_id, step, result
        std::array<int, 3> output;
        output[0] = task_id;

        // Execute each step
        for (int step = stepBeg; step < stepEnd; ++step) {

            // Execute the task
            output[1] = step;

            // Perform the work
            output[2] = this->taskFunc(task_id);

            // Send the result
            MPI_Send(output.data(), 3, MPI_INT, managerRank, endTaskTag, this->comm);
        }

        // Notify the manager that this worker is available again
        int done = 1;
        MPI_Send(&done, 1, MPI_INT, managerRank, workerAvailableTag, MPI_COMM_WORLD);
    }
 
}
