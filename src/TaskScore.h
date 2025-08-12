#include <mpi.h>
#include <vector>
#include <set>

#ifndef TASK_SCORE
#define TASK_SCORE

/**
 * Class Taskscore
 * @brief The Taskscore assigns tasks to TaskWorkers.
 */

class TaskScore {

    private:

        // Communicator
        MPI_Comm comm; 

        // number of tasks
        int numTasks;

        // scoreboard 
        std::vector<int> task_id;
        std::vector<int> status;

        // the MPI window to receive the scores from remote processses
        MPI_Win win;

    public:

        enum Status {
            PENDING = -1,
            RUNNING = -2,
            SUCCEEDED = -3,
            FAILED = -4,
        };

        /**
         * Constructor
         * @param comm communicator
         * @param numTasks number of tasks
         */
        TaskScore(MPI_Comm comm, int numTasks);

        /**
         * Get the IDs of the tasks with status condition
         * @param status either PENDING, RUNNING, SUCCEEDED or FAILED
         * @return set of IDs
         */
        std::set<int> get(int status) const;

};

#endif // TASK_SCORE
