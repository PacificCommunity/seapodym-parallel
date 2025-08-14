#include "TaskStepWorker.h"
#include <array>

TaskStepWorker::TaskStepWorker(MPI_Comm comm, std::function<int(int)> taskFunc) {
    this->comm = comm;
    this->taskFunc = taskFunc;
}
        
void
TaskStepWorker::run(int numSteps) const {

    const int manager_rank = 0;
    int workerId;
    MPI_Comm_rank(this->comm, &workerId);

    int taskId;
    const int startTaskTag = 1;
    const int endTaskTag = 2;

    while (true) {

        // get the assigned task
        int ier = MPI_Recv(&taskId, 1, MPI_INT, manager_rank, startTaskTag, this->comm, MPI_STATUS_IGNORE);
        
        if (taskId < 0) {
            // No more tasks
            break;
        }

        // TaskId, step, result
        std::array<int, 3> output;

        for (output[1] = 0; output[1] < numSteps; ++output[1]) {

            output[0] = taskId;

            // execute the task
            output[2] = this->taskFunc(taskId);

            // send the (taskId, step, result) back to the manager
            ier = MPI_Send(output.data(), output.size(), MPI_INT, manager_rank, endTaskTag,  this->comm);
        }
    }
}
