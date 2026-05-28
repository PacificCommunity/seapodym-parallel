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
        DataProvider dataProvider(MPI_COMM_WORLD);

        int shmRank = dataProvider.getShmRank();

        // on this rank
        double* dataPtr = nullptr;
        size_t numDataPtr = 0;
        std::vector<double> localData; // make sure localData lives until the end of the scope, so that the data pointer remains valid

        // actual data size on the shmRoot rank
        size_t numData = (size_t) cmdLine.get<int>("-nd");

        if (dataProvider.isShmRoot()) {

            numDataPtr = numData;
            localData.resize(numData);

            // Only the shmRoot rank on each shared-memory node initializes the data. There are as many shmRoot
            // ranks as there are nodes.
            for (auto i = 0; i < numData; ++i) {
                localData[i] = (double) (i + 1);
            }
            // set the pointer
            dataPtr = localData.data();
        }

        // associate the data pointer with the DataProvider. 
        // This will set up the shared memory window and make the data available to all ranks on the same node.   
        dataProvider.setDataPtr(dataPtr, numData);

        // Now each rank can get the pointer to the shared data array. 
        const double* shmDataPtr = dataProvider.getDataPtr();

        for (auto i = 0; i < numData; ++i) {
                std::cout
                << "worldRank=" << worldRank
                << " shmRank=" << shmRank
                << " i=" << i
                << " value=" << std::setw(8)
                << shmDataPtr[i]
                << "\n";
        }

        // double checksum = std::accumulate(shmDataPtr, shmDataPtr + numData, 0.0);
        // double expectedChecksum = 0.5 * (numData + 1) * numData; // sum of 1, 2, ..., numData

        // std::cout
        //     << "worldRank=" << worldRank
        //     << " shmRank=" << shmRank
        //     << " checksum=" << checksum
        //     << " expectedChecksum=" << expectedChecksum
        //     << "\n";

    }


    MPI_Finalize();

    std::cout << "Success\n";

    return 0;
}