#include <mpi.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <numeric>
#include <CmdLineArgParser.h>
#include "DataProvider.h"

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    int worldRank, worldSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    // Parse the command line arguments
    CmdLineArgParser cmdLine;
    cmdLine.set("-nd", 5, "Number of data values in shared array");
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

    // Make sure the DataProvider destructor is called before MPI_Finalize
    {
        const int numData = cmdLine.get<int>("-nd");
        DataProvider dataProvider(MPI_COMM_WORLD, numData);

        // get the pointer to the shared data array
        double* shmDataPtr = dataProvider.getDataPtr();

        if (dataProvider.isShmRoot()) {

            // Only the shmRoot rank on each shared-memory node initializes the data. There are as many shmRoot
            // ranks as there are nodes.
            for (auto i = 0; i < numData; ++i) {
                shmDataPtr[i] = (double) (i + 1);
            }
        }

        // synchronize shared-memory access to make sure the data are visible to all ranks on the same node
        MPI_Barrier(MPI_COMM_WORLD);

        // from now on, all ranks on the same shared-memory node can directly read the shared data array 
        // without MPI communication
        double checksum = std::accumulate(shmDataPtr, shmDataPtr + numData, 0.0);
        double expectedChecksum = 0.5 * (numData + 1) * numData; // sum of 1, 2, ..., numData

        // get the rank of the calling process in the shared memory communicator
        int shmRank = dataProvider.getShmRank();

        if (checksum != expectedChecksum) {
            std::cerr
                << "Error: worldRank=" << worldRank
                << " shmRank=" << shmRank
                << " checksum=" << checksum
                << " does not match expectedChecksum=" << expectedChecksum
                << "\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

    }


    if (worldRank == 0) {
        std::cout << "Success\n";
    }

    MPI_Finalize();


    return 0;
}