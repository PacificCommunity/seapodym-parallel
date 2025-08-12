#include "TaskScore.h"

TaskScore::TaskScore(MPI_Comm comm, int numTasks) {
    this->comm = comm;
    this->numTasks = numTasks;
    this->task_id.resize(numTasks);
    this->status.resize(numTasks);
    for (auto i = 0; i < numTasks; ++i) {
        this->task_id[i] = i;
        this->status[i] = PENDING;
    }

    this->entry[0] = -1; this->entry[1] = -1;
    MPI_Win_create(this->entry, 2*sizeof(int), sizeof(int),
                   MPI_INFO_NULL, comm, &this->win);
}

std::set<int> 
TaskScore::get(int status) const {
    std::set<int> result;
    for (auto i = 0; i < this->numTasks; ++i) {
        if (this->status[i] == status) {
            result.insert(i);
        }
    }
    return result;
}
