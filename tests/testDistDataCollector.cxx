#include <DistDataCollector.h>
#include <iostream>
#include<cassert>
#include "CmdLineArgParser.h"
#include <limits>

void test(int numSize, int numChunksPerRank) {

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    int numChunks = numChunksPerRank * nprocs;

    DistDataCollector ddc(MPI_COMM_WORLD, numChunks, numSize);

    if (rank > 0) {
        for (auto i = 0; i < numChunksPerRank; ++i) {
            int chunkId = rank * numChunksPerRank + i;
            std::vector<double> localData(numSize, chunkId);
            std::cout << "[" << rank << "] put chunk " << chunkId << ": ";
            for (auto val : localData) {
                std::cout << val << ", ";
            }
            std::cout << std::endl;
            ddc.put(chunkId, localData.data());

            // At this point the data have been sent and we can modify the local data
            localData.clear();
        }
    }

    // Make sure all the chunks have been received. All the above "put" operations
    // must have completed. 
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        double* collectedData = ddc.getCollectedDataPtr();
        for (auto j = 0; j < numChunks; ++j) {
            std::cout << "[" << rank << "] chunk " << j << ": ";
            for (auto i = 0; i < numSize; ++i) {
                std::cout << collectedData[j * numSize + i] << ", ";
            }
            std::cout << std::endl;
        }
        std::cout << "Success\n";
    }

    if (rank > 0) {
        for (auto i = 0; i < numChunksPerRank; ++i) {
            int chunkId = rank * numChunksPerRank + i;
            std::vector<double> localData = ddc.get(chunkId);
            std::cout << "[" << rank << "] get chunk " << chunkId << ": ";
            for (auto val : localData) {
                std::cout << val << ", ";
                assert(val == chunkId);
            }
            std::cout << std::endl;
        }
    }

    ddc.free();
}


int main(int argc, char* argv[]) {

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    int numSize = 10;
    int numChunksPerRank = 2;
    test(numSize, numChunksPerRank);

    // Finalize the MPI environment
    MPI_Finalize();
    return 0;
}