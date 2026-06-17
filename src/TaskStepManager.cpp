#include "TaskStepManager.h"
#include "Tags.h"
#include <set>
#include <unordered_set>
#include <list>
#include <map>
#include <algorithm>
#include <iostream>

// Hash for std::array<int,2> so it can be used in unordered_set (O(1) lookups).
struct DepHash {
    size_t operator()(const std::array<int,2>& a) const {
        return std::hash<long long>()((long long)a[0] << 32 | (unsigned int)a[1]);
    }
};

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

    int size;
    MPI_Comm_size(this->comm, &size);

    std::set<std::array<int,3>> results;

    // O(1) average lookup vs O(log N) for std::set
    std::unordered_set<std::array<int,2>, DepHash> completed;

    std::set<int> assigned;

    // std::list gives O(1) erase-by-iterator during task assignment
    std::list<int> task_queue;
    for (const auto& [task_id, beg] : this->stepBegMap) task_queue.push_back(task_id);

    std::set<int> active_workers;
    for (int i = 1; i < size; ++i) active_workers.insert(i);

    std::array<int, 3> output;
    MPI_Status status;

    // Receive and process a single message whose tag was already probed.
    auto processMessage = [&](const MPI_Status& st) {
        if (st.MPI_TAG == END_TASK_TAG) {
            MPI_Recv(output.data(), 3, MPI_INT, st.MPI_SOURCE, END_TASK_TAG,
                     this->comm, MPI_STATUS_IGNORE);
            results.insert(output);
            int task_id = output[0];
            int step    = output[1];
            completed.insert({task_id, step});
            if (step == this->stepEndMap.at(task_id) - 1)
                assigned.erase(task_id);
        } else { // WORKER_AVAILABLE_TAG
            int dummy;
            MPI_Recv(&dummy, 1, MPI_INT, st.MPI_SOURCE, WORKER_AVAILABLE_TAG,
                     this->comm, MPI_STATUS_IGNORE);
            active_workers.insert(st.MPI_SOURCE);
        }
    };

    while (!task_queue.empty() || !assigned.empty()) {

        // --- Non-blocking drain: receive everything currently queued (both tags) ---
        bool received_any = false;
        {
            int flag = 0;
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, this->comm, &flag, &status);
            while (flag) {
                processMessage(status);
                received_any = true;
                MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, this->comm, &flag, &status);
            }
        }

        // --- Assign all ready tasks to available workers ---
        for (auto it = task_queue.begin();
             it != task_queue.end() && !active_workers.empty(); ) {
            int task_id = *it;
            const auto& task_deps = this->deps.at(task_id);
            bool ready = std::all_of(task_deps.begin(), task_deps.end(),
                [&](const dep_type& d) { return completed.count(d) > 0; });
            if (ready) {
                int worker = *active_workers.begin();
                active_workers.erase(active_workers.begin());
                MPI_Send(&task_id, 1, MPI_INT, worker, START_TASK_TAG, this->comm);
                assigned.insert(task_id);
                it = task_queue.erase(it);
            } else {
                ++it;
            }
        }

        // --- Block until the next message if there is nothing else to do ---
        // This eliminates the hot-spin when all workers are busy and no
        // messages have arrived yet.  assigned.empty() is impossible here
        // (the outer while would have exited), so a blocking probe is safe.
        if (!received_any && !assigned.empty()) {
            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, this->comm, &status);
            processMessage(status);
        }
    }

    // Shutdown all workers
    const int stop = 0;
    for (int worker = 1; worker < size; ++worker) {
        std::cout << "[Manager] shutting down worker " << worker << "\n";
        MPI_Send(&stop, 1, MPI_INT, worker, SHUTDOWN_TAG, this->comm);
    }

    return results;
}
