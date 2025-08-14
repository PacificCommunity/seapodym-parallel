#include "TaskWorker.h"

TaskWorker::TaskWorker(MPI_Comm comm, std::function<int(int)> taskFunc) {
    this->comm = comm;
    this->taskFunc = taskFunc;
}
        
void
TaskWorker::run() const {

    const int manager_rank = 0;
    int workerId;
    MPI_Comm_rank(this->comm, &workerId);

    int taskId;
    const int startTaskTag = 1;

    while (true) {

        // get the assigned task
        int ier = MPI_Recv(&taskId, 1, MPI_INT, manager_rank, startTaskTag, this->comm, MPI_STATUS_IGNORE);
        
        if (taskId < 0) {
            // No more tasks
            break;
        }

        // execute the task
        int result = this->taskFunc(taskId);

        // send the result, use the taskId as tag
        ier = MPI_Send(&result, 1, MPI_INT, manager_rank, taskId,  this->comm);
    }
}
