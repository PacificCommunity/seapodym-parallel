#include "TaskStepManager.h"
#include <set>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <iostream>


TaskStepManager::TaskStepManager(MPI_Comm comm, int numTasks, 
      const std::map<int, int>& stepBegMap, 
      const std::map<int, int>& stepEndMap,
      const std::map<int, std::set<dep_type>>& dependencyMap) {

    this->comm = comm;
    this->numTasks = numTasks;
    this->stepBegMap = stepBegMap;
    this->stepEndMap = stepEndMap;
    this->deps = dependencyMap;
}

std::set< std::array<int, 3> >
TaskStepManager::run() const {

    // tags the worker will use when communicating with the manager
    const int startTaskTag = 0;
    const int endTaskTag = 1;
    const int workerAvailableTag = 2;
    const int managerRank = 0;

    int size;
    MPI_Comm_size(this->comm, &size);

    std::set<std::array<int,3>> results;
    std::set<std::array<int,2>> completed;

    std::set<int> assigned;
    std::vector<int> task_queue(this->numTasks);
    for (int i = 0; i < this->numTasks; ++i) {
        task_queue[i] = i;
    }
    std::set<int> active_workers;

    // task_id, step, result
    std::array<int, 3> output;

    // Declare all workers to be active initially
    for (int i = 1; i < size; ++i) {
        active_workers.insert(i);
    }

    while (!task_queue.empty() || !assigned.empty()) {

        MPI_Status status;

        // Drain all step-complete messages (tag 1)
        while (true) {

            int flag = 0;
            // Probe for messages from workers
            MPI_Iprobe(MPI_ANY_SOURCE, 1, this->comm, &flag, &status);
            if (!flag) break;

            MPI_Recv(output.data(), 3, MPI_INT, status.MPI_SOURCE, endTaskTag, this->comm, MPI_STATUS_IGNORE);

            // Store the result
            results.insert(output);
            int task_id = output[0];
            int step = output[1];
            completed.insert(std::array<int,2>{task_id, step});

            if (step == this->stepEndMap.at(task_id) - 1) {
                assigned.erase(task_id);
            }
        }

        // Assign ready tasks to any available worker
        for (auto it = task_queue.begin(); it != task_queue.end();) {
            int task_id = *it;
            const auto& deps = this->deps.at(task_id);
            bool ready = true;
            for (auto& dep : deps) {
                if (completed.find(dep) == completed.end()) {
                    ready = false;
                    break;
                }
            }
            if (ready && !active_workers.empty()) {
                int worker = *active_workers.begin();
                active_workers.erase(worker);
                MPI_Send(&task_id, 1, MPI_INT, worker, startTaskTag, this->comm);
                assigned.insert(task_id);
                it = task_queue.erase(it);
            } else {
                ++it;
            }
        }

        // Drain all worker-available messages (tag 2)
        for (int worker = 1; worker < size; ++worker) {
            int flag = 0;
            MPI_Iprobe(worker, 2, this->comm, &flag, &status);
            if (flag) {
                int dummy;
                MPI_Recv(&dummy, 1, MPI_INT, worker, workerAvailableTag, this->comm, MPI_STATUS_IGNORE);
                active_workers.insert(worker);
            }
        }

        // (optional) avoid hot-spinning if nothing to do
        if (active_workers.empty() && assigned.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // Send stop signal
    const int stop = -1;
    for (int worker = 1; worker < size; ++worker) {
        std::cout << "[Manager] shutting down worker " << worker << std::endl;
        MPI_Send(&stop, 1, MPI_INT, worker, startTaskTag, this->comm);
    }

    return results;
}
