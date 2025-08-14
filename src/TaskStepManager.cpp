#include "TaskStepManager.h"
#include <set>
#include <vector>

bool isReady(int taskId, 
    const std::map< int, std::set<dep_type> >& dependencies, 
    const std::set<dep_type>& completed) {

    for (auto dep : dependencies.at(taskId)) {
        if (completed.find(dep) == completed.end()) {
            return false;
        }
    }
    return true;
}

std::vector<int>
getReadyTasks(const std::map< int, std::set<dep_type> >& dependencies,
                            const std::set<dep_type>& completed,
                            const std::set<int>& assigned) {

    std::vector<int> ready;
    for (const auto& [taskId, deps] : dependencies) {
        if (assigned.find(taskId) == assigned.end() && isReady(taskId, dependencies, completed)) {
            ready.push_back(taskId);
        }
    }
    return ready;
}

TaskStepManager::TaskStepManager(MPI_Comm comm, int numTasks) {

    this->comm = comm;
    this->numTasks = numTasks;
}

void
TaskStepManager::addDependencies(int taskId, const std::set<dep_type>& otherTaskIds) {
    this->deps.insert( std::pair<int, std::set<dep_type> >(taskId, otherTaskIds) );
}

std::set< std::array<int, 3> >
TaskStepManager::run() const {

    const int startTaskTag = 1;
    const int shutdown = -1;
    int size;
    int ier = MPI_Comm_size(this->comm, &size);
    const int numWorkers = size - 1;

    std::set<dep_type> completed;
    std::set<int> assigned;
    std::set< std::array<int, 3> > results;

    std::vector<int> ready = getReadyTasks(this->deps, completed, assigned);

    // Initial assignment
    for (int workerId = 1; workerId < size && !ready.empty(); ++ workerId) {
        int taskId = ready.back(); 
        ready.pop_back();
        MPI_Send(&taskId, 1, MPI_INT, workerId, startTaskTag, this->comm);
        assigned.insert(taskId);
    }

    std::array<int, 3> output; // taskId, step, result

    // Receive the results and reassign new tasks
    while (completed.size() < this->numTasks) {

        MPI_Status status;
        MPI_Recv(output.data(), output.size(), MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        int workerId = status.MPI_SOURCE;
        int taskId = output[0];
        int step = output[1];

        results.insert(output);
        completed.insert( dep_type{taskId, step} );
        
        std::cout << "Tasks completed so far: ";
        for (auto [tid, step] : completed) std::cout << tid << ":" << step << ", ";
        std::cout << std::endl;

        ready = getReadyTasks(this->deps, completed, assigned);

        if (!ready.empty()) {
            int nextTaskId = ready.back(); 
            ready.pop_back();
            MPI_Send(&nextTaskId, 1, MPI_INT, workerId, startTaskTag, MPI_COMM_WORLD);
            assigned.insert(nextTaskId);
        }
    }

    // Shutdown
    for (int workerId = 1; workerId < size; ++workerId) {
        MPI_Send(&shutdown, 1, MPI_INT, workerId, startTaskTag, this->comm);
    } 
    
    return results;
}
