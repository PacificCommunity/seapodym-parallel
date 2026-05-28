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
        double* data = nullptr;
        int numData = 0;

        if (dataProvider.isShmRoot()) {

            // Only the shmRoot rank on each shared-memory node initializes the data. There are as many shmRoot
            // ranks as there are nodes.
            numData = cmdLine.get<int>("-nd");
            std::vector<double> localData(numData);
            for (int i = 0; i < numData; ++i)
            {
                localData[i] = worldRank * 1000.0 + i;
            }
            data = localData.data();
        }
        dataProvider.setDataPtr(data, numData);

        // Now each rank can get the pointer to the shared data array. 
        const double* shmDataPtr = dataProvider.getDataPtr();

        double checksum = std::accumulate(shmDataPtr, shmDataPtr + numData, 0.0);

        std::cout
            << "worldRank=" << worldRank
            << " shmRank=" << shmRank
            << " checksum=" << checksum
            << "\n";
 }

    MPI_Finalize();

    std::cout << "Success\n";

    return 0;
}