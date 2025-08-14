#include "TaskDependencyManager.h"
#include <set>
#include <vector>

bool isReady(int taskId, const std::map<int, std::set<int> >& dependencies, const std::set<int>& completed) {
    for (int dep : dependencies.at(taskId)) {
        if (completed.find(dep) == completed.end()) {
            std::cout << "Waiting for " << dep << " to complete before starting " << taskId << std::endl;
            return false;
        }
    }
    std::cout << "Task " << taskId << " can start " << std::endl;
    return true;
}

std::vector<int>
getReadyTasks(const std::map<int, std::set<int> >& dependencies,
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
TaskDependencyManager::addDependencies(int taskId, const std::set<int>& otherTaskIds) {
    this->deps.insert( std::pair<int, std::set<int> >(taskId, otherTaskIds) );
}

std::map<int, int>
TaskDependencyManager::run() const {

    const int startTaskTag = 1;
    const int shutdown = -1;
    int size;
    int ier = MPI_Comm_size(this->comm, &size);
    const int numWorkers = size - 1;

    std::set<int> completed;
    std::set<int> assigned;
    std::map<int, int> results;

    std::vector<int> ready = getReadyTasks(this->deps, completed, assigned);
    for (auto tid : ready) {
        std::cout << tid << " is ready\n";
    }

    // Initial assignment
    for (int workerId = 1; workerId < size && !ready.empty(); ++ workerId) {
        int taskId = ready.back(); 
        ready.pop_back();
        std::cout << "Initially sending task " << taskId << " to worker " << workerId << std::endl;
        MPI_Send(&taskId, 1, MPI_INT, workerId, startTaskTag, this->comm);
        assigned.insert(taskId);
    }

    // Receive the results and reassign new tasks
    int res;
    while (completed.size() < this->numTasks) {
        MPI_Status status;
        MPI_Recv(&res, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        int workerId = status.MPI_SOURCE;
        int taskId = status.MPI_TAG;
        std::cout << "Received result " << res << " from worker " << workerId << std::endl;
        results.insert( std::pair<int, int>(taskId, res) );
        completed.insert(taskId);
        std::cout << "Tasks completed so far: ";
        for (auto tid : completed) std::cout << tid << ", ";
        std::cout << std::endl;

        ready = getReadyTasks(this->deps, completed, assigned);
        std::cout << "Next tasks: "; 
        for (auto tid : ready) {
            std::cout << tid << ", ";
        }
        std::cout << std::endl;

        if (!ready.empty()) {
            int nextTaskId = ready.back(); 
            ready.pop_back();
            std::cout << "Send next task " << nextTaskId << " to worker " << workerId << std::endl;
            MPI_Send(&nextTaskId, 1, MPI_INT, workerId, startTaskTag, MPI_COMM_WORLD);
            assigned.insert(nextTaskId);
        }
    }

    // Shutdown
    for (int workerId = 1; workerId < size; ++workerId) {
        std::cout << "Sending shutdown signal to worker " << workerId << std::endl;
        MPI_Send(&shutdown, 1, MPI_INT, workerId, startTaskTag, this->comm);
    } 
    
    return results;
}
