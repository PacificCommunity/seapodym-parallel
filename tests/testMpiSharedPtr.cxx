#include <mpi.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <CmdLineArgParser.h>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    int worldRank, worldSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    // Parse the command line arguments
    CmdLineArgParser cmdLine;
    cmdLine.set("-nd", 5, "Number of data values in each chunk");
    bool success = cmdLine.parse(argc, argv);
    bool help = cmdLine.get<bool>("-help") || cmdLine.get<bool>("-h");
    if (!success) {
        std::cerr << "Error parsing command line arguments." << std::endl;
        cmdLine.help();
        MPI_Finalize();
        return 1;
    }
    if (help) {
        cmdLine.help();
        MPI_Finalize();
        return 1;
    }

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
    const int numChunks = shmSize;
    const int numData   = cmdLine.get<int>("-nd");

    const MPI_Aint localSize =
        (shmRank == 0)
        ? numChunks * numData * sizeof(double)
        : 0;

    double* baseptr = nullptr;

    MPI_Win win;

    // Rank 0 allocates the shared memory segment, all other ranks specify size 0
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

    // Every rank can now directly read the shared array (including rank 0)
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

    std::cout << "Success\n";

    return 0;
}