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
    TaskScore score(MPI_COMM_WORLD, numTasks);

    if (workerId != 0) {
            // Worker scores
            score.store(0, TaskScore::PENDING);
            score.store(1, TaskScore::RUNNING);
            score.store(2, TaskScore::FAILED);
            score.store(3, TaskScore::SUCCEEDED);
    }

    // Make sure all the puts have completed
    MPI_Barrier(MPI_COMM_WORLD);

    score.print();
    score.free();

    // Clean up
    MPI_Finalize();
    return 0;
}
