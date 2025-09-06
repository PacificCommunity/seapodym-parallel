#include "TaskManager.h"
#include "Tags.h"

TaskManager::TaskManager(MPI_Comm comm, int numTasks) {

    this->comm = comm;
    this->numTasks = numTasks;
}

std::map<int, int>
TaskManager::run() const {

    const int shutdown = -1;
    int size;
    int ier = MPI_Comm_size(this->comm, &size);
    const int numWorkers = size - 1;
    int task_id = 0;
    int res;
    
    // initial distribution of tasks
    for (int rank = 1; rank < numWorkers + 1; ++rank) {
        if (task_id >= 0 && task_id <= this->numTasks) {
            ier = MPI_Send(&task_id, 1, MPI_INT, rank, START_TASK_TAG, this->comm);
            ++task_id;
        }  
    }

    // collect and reassign tasks
    std::map<int, int> results;
    while (task_id < this->numTasks) {

        MPI_Status status;
        ier = MPI_Recv(&res, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, this->comm, &status);
        int taskId = status.MPI_TAG;
        results.insert( std::pair<int, int>(taskId, res) );

        // send the next task
        ier = MPI_Send(&task_id, 1, MPI_INT, status.MPI_SOURCE, START_TASK_TAG, this->comm);

        ++task_id;
    }

    // finish up
    for (int rank = 1; rank < numWorkers + 1; ++rank) {

        MPI_Status status;
        ier = MPI_Recv(&res, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, this->comm, &status);
        int taskId = status.MPI_TAG;
        results.insert( std::pair<int, int>(taskId, res) );

        // send shutdown signal
        ier = MPI_Send(&shutdown, 1, MPI_INT, status.MPI_SOURCE, START_TASK_TAG, this->comm);
    }

    return results;
}
