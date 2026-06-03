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

        std::vector<std::string> names = {"un", "vn", "tempn", "oxygen"};
        std::size_t n = static_cast<std::size_t>(numData);
        std::vector<std::size_t> nsizes = {n, n, n, n};
        DataProvider dataProvider(MPI_COMM_WORLD, names, nsizes);

        // set the field values
        if (dataProvider.isShmRoot()) {
            for (size_t i = 0; i < names.size(); ++i) {
                double* dataPtr = dataProvider.getDataPtr(names[i]);
                std::size_t numData = dataProvider.getNumElements(names[i]);
                for (auto j = 0; j < numData; ++j) {
                    // fill the data with some values, e.g., 1, 2, ..., numData for the first array, 2, 4, ..., 2*numData for the second array, etc.
                    dataPtr[j] = (double) (j + 1) * (i + 1);
                }
            }
        }

        // synchronize shared-memory access to make sure the data are visible to all ranks on the same node
        MPI_Barrier(dataProvider.getShmComm());

        // from now on, all ranks on the same shared-memory node can directly read the shared data array 
        // without MPI communication
        double checksum = 0.0;
        for (auto name : names) {
            double* dataPtr = dataProvider.getDataPtr(name);
            checksum += std::accumulate(dataPtr, dataPtr + dataProvider.getNumElements(name), 0.0);
        }
        std::size_t numFields = names.size();
        double expectedChecksum = 0.5 * (numData + 1) * numData * 0.5 * (numFields + 1) * numFields;

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