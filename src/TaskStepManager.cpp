#include "TaskStepManager.h"
#include "Tags.h"
#include <set>
#include <unordered_set>
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

//std::unordered_set< std::array<int, 3> >
std::set< std::array<int, 3> >
TaskStepManager::run() const {

    int size;
    MPI_Comm_size(this->comm, &size);

    // task_id, step, return code
    //std::unordered_set<std::array<int,3>> results;
    std::set<std::array<int,3>> results;

    // task_id, step
    std::unordered_set<std::array<int,2>> completed;

    std::unordered_set<int> assigned;
    //std::unordered_set<int> task_queue;
    std::vector<int> task_queue(this->numTasks);
    for (int i = 0; i < this->numTasks; ++i) {
        //task_queue.insert(i);
        task_queue[i] = i;
    }
    std::unordered_set<int> active_workers;

    // task_id, step, result
    std::array<int, 3> output;

    // Declare all workers to be active initially
    for (int worker = 1; worker < size; ++worker) {
        active_workers.insert(worker);
    }

    while (!task_queue.empty() || !assigned.empty()) {

        MPI_Status status;

        // Drain all step-complete messages (tag END_TASK_TAG)
        while (true) {

            int flag = 0;
            // Probe for messages from workers
            MPI_Iprobe(MPI_ANY_SOURCE, END_TASK_TAG, this->comm, &flag, &status);
            if (!flag) {
                // not an end task message
                break;
            }

            MPI_Recv(output.data(), 3, MPI_INT, status.MPI_SOURCE, END_TASK_TAG, this->comm, MPI_STATUS_IGNORE);

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
                MPI_Send(&task_id, 1, MPI_INT, worker, START_TASK_TAG, this->comm);
                assigned.insert(task_id);
                it = task_queue.erase(it);
            } else {
                ++it;
            }
        }

        // Drain ALL worker-available messages in a single loop
        while (true) {
            int flag = 0;
            MPI_Status status;
            MPI_Iprobe(MPI_ANY_SOURCE, WORKER_AVAILABLE_TAG, this->comm, &flag, &status);
            if (!flag) {
                break;  // No more messages
            }

            int dummy;
            int worker = status.MPI_SOURCE;  // Extract worker rank from status
            MPI_Recv(&dummy, 1, MPI_INT, worker, WORKER_AVAILABLE_TAG, this->comm, MPI_STATUS_IGNORE);
            active_workers.insert(worker);
        }

        // avoid hot-spinning if nothing to do
        if (active_workers.empty() && assigned.empty()) {
           // 2-10 ms seems to be a good choice for now. On power constrained 
           // platforms may want to increase to 20-50ms
           std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

    // Send stop signal
    const int stop = -1;
    for (int worker = 1; worker < size; ++worker) {
        std::cout << "[Manager] shutting down worker " << worker << std::endl;
        MPI_Send(&stop, 1, MPI_INT, worker, START_TASK_TAG, this->comm);
    }

    return results;
}
