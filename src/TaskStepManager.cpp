#include "TaskStepManager.h"
#include "Tags.h"
#include <set>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <iostream>


TaskStepManager::TaskStepManager(MPI_Comm comm, int numTasks, 
      const std::map<int, int>& stepBegMap, 
      const std::map<int, int>& stepEndMap,
      const std::map<int, std::set<dep_type>>& dependencyMap,
      DistDataCollector* dataCollect,
    const std::map< std::array<int, 2>, int >& chunkIdMap) {

    this->comm = comm;
    this->numTasks = numTasks;
    this->stepBegMap = stepBegMap;
    this->stepEndMap = stepEndMap;
    this->deps = dependencyMap;
    this->dataCollect = dataCollect;
    this->chunkIdMap = chunkIdMap;
}

std::set< std::array<int, 3> >
TaskStepManager::run() const {

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

    MPI_Status status;
    double* managerData = this->dataCollect->getCollectedDataPtr();
    int numData = this->dataCollect->getNumSize();

    while (!task_queue.empty() || !assigned.empty()) {



        // Check for newly completed steps by scanning the managerData array
        for (int task_id = 0; task_id < this->numTasks; ++task_id) {
            for (int step = this->stepBegMap.at(task_id);
                     step < this->stepEndMap.at(task_id); ++step) {

                std::array<int,2> key{task_id, step};
                if (completed.find(key) != completed.end()) {
                    continue; // already marked
                }

                int chunk_id = this->chunkIdMap.at(key);

                // ensure local window is up to date
                //MPI_Win_sync(dataCollect->win);
                
                double lastVal = managerData[chunk_id * numData + (numData - 1)];
                if (lastVal != this->dataCollect->BAD_VALUE) {
                    // Step is done
                    completed.insert(key);
                    results.insert({task_id, step, /*success*/ 1});

                    if (step == this->stepEndMap.at(task_id) - 1) {
                        assigned.erase(task_id);
                    }
                }
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

        // Drain all worker-available messages (tag WORKER_AVAILABLE_TAG)
        for (int worker = 1; worker < size; ++worker) {
            int flag = 0;
            MPI_Iprobe(worker, WORKER_AVAILABLE_TAG, this->comm, &flag, &status);
            if (flag) {
                int dummy;
                MPI_Recv(&dummy, 1, MPI_INT, worker, WORKER_AVAILABLE_TAG, this->comm, MPI_STATUS_IGNORE);
                active_workers.insert(worker);
            }
        }

        // (optional) avoid hot-spinning if nothing to do
        //if (active_workers.empty() && assigned.empty()) {
        //    std::this_thread::sleep_for(std::chrono::milliseconds(1));
        //}
    }

    // Send stop signal
    const int stop = -1;
    for (int worker = 1; worker < size; ++worker) {
        std::cout << "[Manager] shutting down worker " << worker << std::endl;
        MPI_Send(&stop, 1, MPI_INT, worker, START_TASK_TAG, this->comm);
    }

    return results;
}
