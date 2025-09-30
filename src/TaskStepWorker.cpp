#include "TaskStepWorker.h"
#include "Tags.h"
#include <array>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

TaskStepWorker::TaskStepWorker(MPI_Comm comm, 
  std::function<void(int, int, int, MPI_Comm)> taskFunc,
  const std::map<int, int>& stepBegMap, 
  const std::map<int, int>& stepEndMap) {
    this->comm = comm;
    this->taskFunc = taskFunc;
    this->stepBegMap = stepBegMap;
    this->stepEndMap = stepEndMap;
    MPI_Comm_rank(comm, &this->rank);
}
        
void
TaskStepWorker::run() const {

    const int managerRank = 0;

    std::string sworkerId = std::to_string(this->rank);
    // Use true to let logs be overwritten, otherwise the logs will be appended
    auto logger = spdlog::basic_logger_mt(sworkerId, "log_worker" + sworkerId + ".txt", true);
    logger->set_level(spdlog::level::debug);
    logger->info("Starting loop");
    while (true) {

        // Get the task_id to operate on
        int task_id;
        logger->info("Waiting for manager to send a task...");
        MPI_Recv(&task_id, 1, MPI_INT, managerRank, START_TASK_TAG, this->comm, MPI_STATUS_IGNORE);
        logger->info("Received task {}", task_id);

        if (task_id == -1) {
            // Shutdown
            logger->info("Shutting down.");
            break;
        }

        int stepBeg = this->stepBegMap.at(task_id);
        int stepEnd = this->stepEndMap.at(task_id);

        // Perform the task, which includes stepping from stepBeg to stepEnd - 1.
        // This function should notify the manager at the end of each step
        logger->info("Running task {} from step {} to {}", task_id, stepBeg, stepEnd);
        this->taskFunc(task_id, stepBeg, stepEnd, this->comm);
        logger->info("Finished task {}", task_id);

        // Notify the manager that this worker is available again
        int done = 1;
        logger->info("Notifying manager");
        MPI_Send(&done, 1, MPI_INT, managerRank, WORKER_AVAILABLE_TAG, MPI_COMM_WORLD);
        logger->info("Done.");
    }
 
}
