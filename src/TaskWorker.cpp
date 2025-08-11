#include "TaskWorker.h"

TaskWorker::TaskWorker(MPI_Comm comm, int(*taskFunc)(int)) {
    this->comm = comm;
    this->taskFunc = taskFunc;
}
        
void
TaskWorker::run() const {

    const int manager_rank = 0;

    int task_id;
    const int recvTag = 1;
    const int sendTag = 2;

    while (true) {

        // get thge assigned task
        int ier = MPI_Recv(&task_id, 1, MPI_INT, manager_rank, recvTag, this->comm, MPI_STATUS_IGNORE);

        if (task_id < 0) {
            // No more tasks
            break;
        }

        // execute the task
        int result = this->taskFunc(task_id);

        // send the result
        ier = MPI_Send(&result, 1, MPI_INT, manager_rank, sendTag,  this->comm);
    }
}
