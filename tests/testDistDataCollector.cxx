#include <DistDataCollector.h>
#include <iostream>

void test(int numSize, int numChunksPerRank) {

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    int numChunks = numChunksPerRank * nprocs;

    DistDataCollector ddc(MPI_COMM_WORLD, numChunks, numSize);

    if (rank > 0) {
        for (auto i = 0; i < numChunksPerRank; ++i) {
            int chunkId = rank * numChunksPerRank + i;
            std::vector<double> data(numSize, chunkId);
            ddc.put(chunkId, data.data());
        }
    }

    if (rank == 0) {
        double* collectedData = ddc.getCollectedDataPtr();
        for (auto j = 0; j < numChunks; ++j) {
            std::cout << "Chunk " << j << ": ";
            for (auto i = 0; i < numSize; ++i) {
                std::cout << collectedData[j * numSize + i] << ", ";
            }
            std::cout << std::endl;
        }
        std::cout << "Success\n";
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