#include <mpi.h>
#include <iostream>
#include <vector>
#include <iomanip>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    int worldRank, worldSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    // Create communicator containing ranks on the same shared-memory node
    MPI_Comm shmcomm;
    MPI_Comm_split_type(
        MPI_COMM_WORLD,
        MPI_COMM_TYPE_SHARED,
        0,
        MPI_INFO_NULL,
        &shmcomm);

    int shmRank, shmSize;
    MPI_Comm_rank(shmcomm, &shmRank);
    MPI_Comm_size(shmcomm, &shmSize);

    // Example dimensions
    const int numChunks = 4;
    const int numData   = 8;

    const MPI_Aint localSize =
        (shmRank == 0)
        ? numChunks * numData * sizeof(double)
        : 0;

    double* baseptr = nullptr;

    MPI_Win win;

    // Rank 0 allocates the shared memory segment
    MPI_Win_allocate_shared(
        localSize,
        sizeof(double),
        MPI_INFO_NULL,
        shmcomm,
        &baseptr,
        &win);

    // Other ranks query the shared allocation
    if (shmRank != 0)
    {
        MPI_Aint queriedSize;
        int dispUnit;

        MPI_Win_shared_query(
            win,
            0, // query rank 0's allocation
            &queriedSize,
            &dispUnit,
            &baseptr);
    }

    // Rank 0 initializes the data
    if (shmRank == 0)
    {
        for (int c = 0; c < numChunks; ++c)
        {
            for (int i = 0; i < numData; ++i)
            {
                int idx = c * numData + i;
                baseptr[idx] = 1000.0 * c + i;
            }
        }
    }

    // Synchronize shared-memory access
    MPI_Barrier(shmcomm);

    // Every rank can now directly read the shared array
    for (int c = 0; c < numChunks; ++c)
    {
        for (int i = 0; i < numData; ++i)
        {
            int idx = c * numData + i;

            std::cout
                << "worldRank=" << worldRank
                << " shmRank=" << shmRank
                << " chunk=" << c
                << " i=" << i
                << " value=" << std::setw(8)
                << baseptr[idx]
                << "\n";
        }
    }

    MPI_Barrier(shmcomm);

    MPI_Win_free(&win);
    MPI_Comm_free(&shmcomm);

    MPI_Finalize();
    return 0;
}