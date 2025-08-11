#include "TaskManager.h"

TaskManager::TaskManager(MPI_Comm comm, int numTasks) {

    this->comm = comm;
    this->numTasks = numTasks;
}

std::vector<int>
TaskManager::run() const {

    const int startTaskTag = 1;
    const int endTaskTag = 2;
    const int shutdown = -1;
    int size;
    int ier = MPI_Comm_size(this->comm, &size);
    const int numWorkers = size - 1;
    int task_id = 0;
    int res;
    
    // initial distribution of tasks
    for (int rank = 1; rank < numWorkers + 1; ++rank) {
        task_id = rank - 1;
        if (task_id >= 0 && task_id < this->numTasks) {
            ier = MPI_Send(&task_id, 1, MPI_INT, rank, startTaskTag, this->comm);
        }  
    }

    // collect and reassign tasks
    std::vector<int> results;
    while (task_id < this->numTasks) {

        MPI_Status status;
        ier = MPI_Recv(&res, 1, MPI_INT, MPI_ANY_SOURCE, endTaskTag, this->comm, &status);
        results.push_back(res);

        // send the next task
        ier = MPI_Send(&task_id, 1, MPI_INT, status.MPI_SOURCE, startTaskTag, this->comm);

        task_id++;

    }

    // finish up
    for (int rank = 1; rank < numWorkers + 1; ++rank) {

        MPI_Status status;
        ier = MPI_Recv(&res, 1, MPI_INT, MPI_ANY_SOURCE, endTaskTag, this->comm, &status);
        results.push_back(res);

        // send shutdown signal
        ier = MPI_Send(&shutdown, 1, MPI_INT, status.MPI_SOURCE, MPI_ANY_TAG, this->comm);
    }

    return results;
}
