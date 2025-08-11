#include "TaskWorker.h"

TaskWorker::TaskWorker(MPI_Comm comm, std::function<int(int)> taskFunc) {
    this->comm = comm;
    this->taskFunc = taskFunc;
    // check that I can call the function
    std::cerr << "this->taskFunc(5) = " << this->taskFunc(5) << '\n';
}
        
void
TaskWorker::run() const {

    const int manager_rank = 0;
    int workerId;
    MPI_Comm_rank(this->comm, &workerId);

    int task_id;
    const int startTaskTag = 1;
    const int endTaskTag = 2;

    while (true) {

        // get the assigned task
        int ier = MPI_Recv(&task_id, 1, MPI_INT, manager_rank, startTaskTag, this->comm, MPI_STATUS_IGNORE);
        std::cerr << "[" << workerId << "] recv'ed task " << task_id << '\n';
        
        if (task_id < 0) {
            // No more tasks
            break;
        }

        // execute the task
        int result = this->taskFunc(task_id);

        // send the result
        std::cerr << "[" << workerId << "] sends result " << result << '\n';
        ier = MPI_Send(&result, 1, MPI_INT, manager_rank, endTaskTag,  this->comm);
    }
}
