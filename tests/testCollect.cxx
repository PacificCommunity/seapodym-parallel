#include <mpi.h>
#include <vector>
#include <iostream>
#include <cassert>

int main(int argc, char* argv[]) {

    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    const int numLocal = 10; //15000;       // elements per rank
    const int totalSize = numLocal * nprocs;

    double* collectedData = nullptr;
    MPI_Win win;

    // Allocate the window: only rank 0 allocates real memory
    MPI_Aint winSize = (rank == 0) ? totalSize * sizeof(double) : 0;
    MPI_Win_allocate(winSize, sizeof(double), MPI_INFO_NULL, MPI_COMM_WORLD,
                     &collectedData, &win);

    // Initialize with NaNs on rank 0
    if (rank == 0) {
        for (int i = 0; i < totalSize; ++i) collectedData[i] = std::numeric_limits<double>::quiet_NaN();
    }

    // Start a passive target epoch
    MPI_Win_lock_all(0, win);

    // Prepare local array
    std::vector<double> localData(numLocal, static_cast<double>(rank));

    // Put local data into rank 0 window
    MPI_Put(localData.data(), numLocal, MPI_DOUBLE,
            0, rank * numLocal, numLocal, MPI_DOUBLE, win);

    // Ensure remote completion
    MPI_Win_flush_all(win);

    // Barrier: ensure all ranks have completed their puts
    MPI_Barrier(MPI_COMM_WORLD);

    // Rank 0 checks the results
    if (rank == 0) {
        MPI_Win_sync(win); // sync local memory
        bool success = true;

        for (int r = 0; r < nprocs; ++r) {
            for (int i = 0; i < numLocal; ++i) {
                double val = collectedData[r * numLocal + i];
                if (val != r) {
                    std::cout << "Error: collectedData[" << r * numLocal + i << "] = " << val
                              << " (expected " << r << ")\n";
                    success = false;
                }
            }
        }

        if (success) std::cout << "All values are correct!\n";
        else std::cout << "Some values are incorrect!\n";
    }

    // End epoch and free window
    MPI_Win_unlock_all(win);
    MPI_Win_free(&win);

    MPI_Finalize();
    return 0;
}