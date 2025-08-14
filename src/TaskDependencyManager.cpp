#include "TaskDependencyManager.h"
#include <set>

bool isReady(int taskId, const std::map<int, std::vector<int> >& dependencies, const std::set<int>& completed) {
    for (int dep : dependencies.at(taskId)) {
        if (completed.find(dep) == completed.end()) return false;
    }
    return true;
}

std::vector<int>
getReadyTasks(const std::map<int, std::vector<int> >& dependencies,
                            const std::set<int>& completed,
                            const std::set<int>& assigned) {
    std::vector<int> ready;
    for (const auto& [taskId, deps] : dependencies) {
        if (assigned.find(taskId) == assigned.end() && isReady(taskId, dependencies, completed)) {
            ready.push_back(taskId);
        }
    }
    return ready;
}

TaskDependencyManager::TaskDependencyManager(MPI_Comm comm, int numTasks) {

    this->comm = comm;
    this->numTasks = numTasks;
}

void
TaskDependencyManager::addDependency(int taskId, const std::vector<int>& otherTaskIds) {
    this->deps.insert( std::pair<int, std::vector<int> >(taskId, otherTaskIds) );
}



std::vector<int>
TaskDependencyManager::run() const {

    const int startTaskTag = 1;
    const int endTaskTag = 2;
    const int shutdown = -1;
    int size;
    int ier = MPI_Comm_size(this->comm, &size);
    const int numWorkers = size - 1;
    int task_id = 0;
    int res;

    std::set<int> completed;
    std::set<int> assigned;
    std::vector<int> results;

    std::vector<int> ready = getReadyTasks(this->deps, completed, assigned);
    int activeWorkers = 0;

    // Initial assignment
    for (int workerId = 1; workerId < size && !ready.empty(); ++ workerId) {
        int taskId = ready.back(); ready.pop_back();
	MPI_Send(&taskId, 1, MPI_INT, workerId, startTaskTag, this->comm);
        assigned.insert(taskId);
        activeWorkers++;
    }

    // Receive the results and reassign new tasks
    while (completed.size() < this->deps.size()) {
        int taskId;
        MPI_Status status;
        int res;
        MPI_Recv(&res, 1, MPI_INT, MPI_ANY_SOURCE, endTaskTag, this->comm, &status);
        results.push_back(res);
        int workerId = status.MPI_SOURCE;
        completed.insert(taskId);
        activeWorkers++;
    }

    // Shutdown
    for (int workerId = 1; workerId < size; ++workerId) {
        MPI_Send(&shutdown, 1, MPI_INT, workerId, startTaskTag, this->comm);
    } 
    
    return results;
}
