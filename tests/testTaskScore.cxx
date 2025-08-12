#include <mpi.h>
#include "TaskScore.h"

int main(int argc, char** argv) {

    // MPI initialization
    MPI_Init(&argc, &argv);
    int numWorkers, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    numWorkers = size - 1;
    int workerId;
    MPI_Comm_rank(MPI_COMM_WORLD, &workerId);

    int numTasks = 4;
    int status[2];
    // Ensure score is destroyed before MPI_Finalize is called
    {
        TaskScore score(MPI_COMM_WORLD, numTasks);

        if (workerId == 0) {
            // Score keeper

            std::cout << "Success.\n";
        } else {
            // Worker
            status[0] = 0; status[1] = TaskScore::PENDING;
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, score.win);  // Lock rank 0's window
            MPI_Put(status, 2, MPI_INT, 0, 0, 2, MPI_INT, score.win);
            MPI_Win_unlock(0, score.win);


        }
    }    

    // Clean up
    MPI_Finalize();
    return 0;
}
