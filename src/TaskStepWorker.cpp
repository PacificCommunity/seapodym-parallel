#include "TaskStepWorker.h"
#include "Tags.h"
#include <array>

TaskStepWorker::TaskStepWorker(MPI_Comm comm, 
  std::function<void(int, int, int, MPI_Comm)> taskFunc,
  std::map<int, int> stepBegMap, std::map<int, int> stepEndMap) {
    this->comm = comm;
    this->taskFunc = taskFunc;
    this->stepBegMap = stepBegMap;
    this->stepEndMap = stepEndMap;
}

void TaskStepWorker::run() const {
    const int managerRank = 0;
    int workerId;
    MPI_Comm_rank(this->comm, &workerId);

    while (true) {
        int task_id;
        MPI_Recv(&task_id, 1, MPI_INT, managerRank, START_TASK_TAG, this->comm, MPI_STATUS_IGNORE);

        if (task_id == -1) break; // shutdown

        int stepBeg = this->stepBegMap.at(task_id);
        int stepEnd = this->stepEndMap.at(task_id);

        // Perform the task
        this->taskFunc(task_id, stepBeg, stepEnd, this->comm);

        // Nonblocking notify manager
        int done = 1;
        MPI_Request req;
        MPI_Isend(&done, 1, MPI_INT, managerRank, WORKER_AVAILABLE_TAG, MPI_COMM_WORLD, &req);
        MPI_Request_free(&req); // worker doesn't need to wait
    }
}
