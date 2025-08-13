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

        // local MPI rank
        int me;

        // scoreboard 
        std::vector<int> status;

        // the MPI window to receive the score updates from remote processses
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
         * Store a score
         * @param taskInd task Id
         * @param status one of PENDING, RUNNING, SUCCEEDED or FAILED
         * @note this should be called by the remote process
         */
        void store(int taskId, int status);

        /**
         * Print
         * 
         */
         void print() const;

        /**
         * Free the MPI window
         */
        void free() {
            if (this->win != MPI_WIN_NULL) {
                MPI_Win_free(&this->win);
            }
        }

        /**
        * Destructor
         */
        ~TaskScore() {
            this->free();
        }

        /**
         * Get the IDs of the tasks with status condition
         * @param status one of PENDING, RUNNING, SUCCEEDED or FAILED
         * @return set of IDs
         */
        std::set<int> get(int status) const;

};

#endif // TASK_SCORE
