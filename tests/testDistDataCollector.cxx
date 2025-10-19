#include <DistDataCollector.h>
#include <iostream>
#include "CmdLineArgParser.h"
#undef NDEBUG
#include <cassert>

void test(int numSize, int numChunksPerRank) {

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    int numChunks = numChunksPerRank * nprocs;

    DistDataCollector ddc(MPI_COMM_WORLD, numChunks, numSize);

    double timePut = 0, timeGet = 0;

    if (rank >= 0) {

        double tic = MPI_Wtime();

        for (auto i = 0; i < numChunksPerRank; ++i) {

            int chunkId = rank * numChunksPerRank + i;
            std::vector<double> localData(numSize, chunkId);

            ddc.put(chunkId, localData.data());

            // At this point the data have been sent and we can modify the local data
            localData.clear();
        }

        double toc = MPI_Wtime();
        timePut += toc - tic;
    }

    // Make sure all the chunks have been received. All the above "put" operations
    // must have completed. 
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank > 0) {

        double tic = MPI_Wtime();

        for (auto i = 0; i < numChunksPerRank; ++i) {

            int chunkId = rank * numChunksPerRank + i;
            std::vector<double> localData = ddc.get(chunkId);

            for (auto val : localData) {
                assert(val == chunkId);
            }
        }

        double toc = MPI_Wtime();
        timeGet += toc - tic;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        int numChunk = ddc.getNumChunks();
        int numSize = ddc.getNumSize();
        double* data = ddc.getCollectedDataPtr();
        double checksum = 0;
        for (auto chunk = 0; chunk < numChunk; ++chunk) {
            std::cout << "chunk " << chunk << ": ";
            for (auto i = 0; i < numSize; ++i) {
                std::cout << data[chunk*numSize + i] << ", ";
                checksum += data[chunk*numSize + i];
            }
            std::cout << std::endl;
        }
        std::cout << "checksum: " << checksum << std::endl;
        assert(checksum == numSize * numChunks * (numChunks - 1) / 2);
    }

    ddc.free();

    double timePutTotal, timeGetTotal;
    MPI_Reduce(&timePut, &timePutTotal, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&timeGet, &timeGetTotal, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "Average time put: " << timePutTotal/double(nprocs - 1) << " secs" << std::endl;
        std::cout << "Average time get: " << timeGetTotal/double(nprocs - 1) << " secs" << std::endl;
        std::cout << "Success\n";
    }
}


int main(int argc, char* argv[]) {

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Parse the command line arguments
    CmdLineArgParser cmdLine;
    cmdLine.set("-numSize", 15000, "Size of the data to put/get");
    cmdLine.set("-numChunksPerRank", 1, "Number of chunks per rank");
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

    int numSize = cmdLine.get<int>("-numSize");
    int numChunksPerRank = cmdLine.get<int>("-numChunksPerRank");

    // Run the test
    test(numSize, numChunksPerRank);

    // Finalize the MPI environment
    MPI_Finalize();
    return 0;
}
