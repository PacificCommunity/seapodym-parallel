#include "DistDataCollector.h"
#include <mpi.h>
#include <iostream>
#include <numeric> // std::accumulate
#include <functional>
#include <chrono>
#include <thread>
#include "CmdLineArgParser.h"


void initData(DistDataCollector& dataCollector1, DistDataCollector& dataCollector2, int num_chunks, int num_size) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        // fill the collected array with known values for testing
        double* data1 = dataCollector1.getCollectedDataPtr();
        for (int i = 0; i < num_chunks; ++i) {
            std::fill(data1 + i * num_size, data1 + (i + 1) * num_size, double(i));
        }
        double* data2 = dataCollector2.getCollectedDataPtr();
        std::fill(data2, data2 + num_chunks*num_size, -1.0);
        
    }
}

void checkData(const std::string& testCase, 
    DistDataCollector& dataCollector1, DistDataCollector& dataCollector2, 
    int num_chunks, int num_size) {

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        // verify that dataCollector2 has the same data as dataCollector1
        double* data1 = dataCollector1.getCollectedDataPtr();
        double* data2 = dataCollector2.getCollectedDataPtr();
        double diff = 0.0;
        // we are only checking the data written by other ranks
        for (int i = num_size; i < num_chunks * num_size; ++i) {
            double val1 = data1[i];
            double val2 = data2[i];
            diff += std::abs(val1 - val2);
        }
        if (diff > 1e-10) {
            std::cerr << testCase << " Put/Get test failed, diff = " << diff << "\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
}

/**
 * @brief Test asynchronous put/get with DistDataCollector
 * @param num_chunks number of chunks
 * @param num_size size of each chunk
 * @param ms milliseconds to simulate work
 */
void testAsyncPutGet(int num_chunks, int num_size, int ms) {

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    DistDataCollector dataCollector1(MPI_COMM_WORLD, num_chunks, num_size);
    DistDataCollector dataCollector2(MPI_COMM_WORLD, num_chunks, num_size);

    initData(dataCollector1, dataCollector2, num_chunks, num_size);

    // make sure data has been initialized before remote memory accesses are issued
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank > 0) {
        int chunk_id = rank;
        std::vector<double> localData(num_size);

        dataCollector1.startEpoch();
        dataCollector1.getAsync(chunk_id, localData.data());
        dataCollector1.flush();
        dataCollector1.endEpoch();

        dataCollector2.startEpoch();
        dataCollector2.putAsync(chunk_id, localData.data());
        dataCollector2.flush();
        dataCollector2.endEpoch();
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank > 0) {

        std::vector<double> localData(num_size);
        double checksum = 0.0;

        // this is how one could use getAsync in conjunction with startEpoch/flush/endEpoch
        double tic = MPI_Wtime();
        dataCollector2.startEpoch();
        for (auto i = 0; i < num_chunks; ++i) {

            // fetch a chunk asynchronously
            dataCollector2.getAsync(i, localData.data());

            // .. do some work ...
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));

            // make sure the get is complete before using localData
            dataCollector2.flush();
            checksum += std::accumulate(localData.begin(), localData.end(), 0.0);
        }
        dataCollector2.endEpoch();
        double toc = MPI_Wtime();
        std::cout << " Async Rank " << rank << " checksum = " << checksum << " time: " << (toc - tic) << "sec\n";

    } else {
        // rank 0 could also use getAsync/putAsync in a similar manner
        // but here we just compute the checksum directly
        double* data = dataCollector2.getCollectedDataPtr();
        double checksum = std::accumulate(data, data + num_chunks * num_size, 0.0);
        std::cout << "Rank " << rank << " checksum = " << checksum << std::endl;    
    }

    // check
    checkData("Async", dataCollector1, dataCollector2, num_chunks, num_size);
}

/**
 * @brief Test blocking put/get with DistDataCollector
 * @param num_chunks number of chunks
 * @param num_size size of each chunk
 * @param ms milliseconds to simulate work
 */
void testPutGet(int num_chunks, int num_size, int ms) {

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    DistDataCollector dataCollector1(MPI_COMM_WORLD, num_chunks, num_size);
    DistDataCollector dataCollector2(MPI_COMM_WORLD, num_chunks, num_size);

    initData(dataCollector1, dataCollector2, num_chunks, num_size);

    // make sure data has been initialized before remote memory accesses are issued
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank > 0) {
        int chunk_id = rank;
        std::vector<double> localData(num_size);

        dataCollector1.get(chunk_id, localData.data());

        dataCollector2.put(chunk_id, localData.data());
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank > 0) {

        std::vector<double> localData(num_size);
        double checksum = 0.0;

        double tic = MPI_Wtime();
        for (auto i = 0; i < num_chunks; ++i) {

            // fetch a chunk asynchronously
            dataCollector2.get(i, localData.data());

            // ... do some work ...
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));

            checksum += std::accumulate(localData.begin(), localData.end(), 0.0);
        }
        double toc = MPI_Wtime();
        std::cout << "       Rank " << rank << " checksum = " << checksum << " time: " << (toc - tic) << "sec\n";

    } else {
        // rank 0 could also use getAsync/putAsync in a similar manner
        // but here we just compute the checksum directly
        double* data = dataCollector2.getCollectedDataPtr();
        double checksum = std::accumulate(data, data + num_chunks * num_size, 0.0);
        std::cout << "Rank " << rank << " checksum = " << checksum << std::endl;    
    }

    // check
    checkData("", dataCollector1, dataCollector2, num_chunks, num_size);
}


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Parse the command line arguments
    CmdLineArgParser cmdLine;
    cmdLine.set("-nd", 10000, "Number of data values to send from worker to manager at each step");
    cmdLine.set("-nm", 10, "Sleep milliseconds");
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

    const int num_chunks = size;
    const int num_size = cmdLine.get<int>("-nd"); // size of each chunk
    const int ms = cmdLine.get<int>("-nm");

    testAsyncPutGet(num_chunks, num_size, ms);
    testPutGet(num_chunks, num_size, ms);

    if (rank == 0) {
        std::cout << "Success\n";
    }
    MPI_Finalize();
    return 0;
}