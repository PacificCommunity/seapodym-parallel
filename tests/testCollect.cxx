#include <mpi.h>
#include <vector>
#include <iostream>
#include <cassert>
#include <limits>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    const int numLocal = 15000;
    const int totalSize = numLocal * nprocs;

    double* collectedData = nullptr;
    MPI_Win win;

    MPI_Aint winSize = (rank == 0) ? totalSize * sizeof(double) : 0;
    MPI_Win_allocate(winSize, sizeof(double), MPI_INFO_NULL, MPI_COMM_WORLD,
                     &collectedData, &win);

    // initialize rank 0 memory
    if (rank == 0)
        for (int i=0; i<totalSize; ++i) collectedData[i] = std::numeric_limits<double>::quiet_NaN();

    std::vector<double> localData(numLocal, static_cast<double>(rank));

    // Active-target epoch
    MPI_Win_fence(0, win); // start epoch

    MPI_Put(localData.data(), numLocal, MPI_DOUBLE,
            0, rank*numLocal, numLocal, MPI_DOUBLE, win);

    MPI_Win_fence(0, win); // end epoch, guarantees all puts completed

    // rank 0 reads data
    if (rank == 0) {
        bool success = true;
        for (int r = 0; r < nprocs; ++r)
            for (int i = 0; i < numLocal; ++i)
                if (collectedData[r*numLocal + i] != r) {
                    success = false;
                    std::cout << "Error at rank " << r << ", index " << i << "\n";
                }

        if (success) std::cout << "All values correct!\n";
        else std::cout << "Some values incorrect!\n";
    }

    MPI_Win_free(&win);
    MPI_Finalize();
    return 0;
}