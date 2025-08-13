#include "TaskScore.h"
#include <map>

TaskScore::TaskScore(MPI_Comm comm, int numTasks) {
    this->comm = comm;
    MPI_Comm_rank(MPI_COMM_WORLD, &this->me);
    this->numTasks = numTasks;

    if (this->me == 0) {
        // Initialize the scores
        this->status.resize(numTasks);
        for (auto i = 0; i < numTasks; ++i) {
            this->status[i] = PENDING;
        }
    }

    MPI_Win_create(&this->status[0], numTasks*sizeof(int), sizeof(int),
                   MPI_INFO_NULL, comm, &this->win);
}

void
TaskScore::store(int taskId, int stat) {
    const int managerRank = 0;
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, managerRank, 0, this->win);  // Lock rank 0's window

    MPI_Put(&stat, 1, MPI_INT,
            managerRank, taskId, 1, MPI_INT,
            this->win);

    MPI_Win_unlock(0, this->win);
}

void
TaskScore::print() const {
    std::map<int, std::string> mp;
    mp[PENDING] = "PENDING";
    mp[RUNNING] = "RUNNING";
    mp[FAILED] = "FAILED";
    mp[SUCCEEDED] = "SUCCEEDED";
    MPI_Barrier(this->comm);
    if (this->me == 0) {
        for (auto i = 0; i < this->status.size(); ++i) {
            std::cout << "Task " << i << ": " << mp[ this->status[i] ] << std::endl;
        }
    }
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
