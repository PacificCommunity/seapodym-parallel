#include "DistDataCollector.h"
#include <mpi.h>
#include <iostream>
#include <numeric> // std::accumulate
#include <functional>


void testPutGet(int num_chunks, int num_size) {

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    DistDataCollector dataCollector1(MPI_COMM_WORLD, num_chunks, num_size);
    DistDataCollector dataCollector2(MPI_COMM_WORLD, num_chunks, num_size);

    if (rank == 0) {
        // fill the collected array with known values for testing
        double* data1 = dataCollector1.getCollectedDataPtr();
        for (int i = 0; i < num_chunks; ++i) {
            std::fill(data1 + i * num_size, data1 + (i + 1) * num_size, double(i));
        }
        double* data2 = dataCollector2.getCollectedDataPtr();
        std::fill(data2, data2 + num_chunks*num_size, -1.0);
        
    }

    // make sure all ranks have initialized
    MPI_Barrier(MPI_COMM_WORLD);
    
    if (rank > 0) {
        int chunk_id = rank;
        std::vector<double> localData = dataCollector1.get(chunk_id);
        dataCollector2.put(chunk_id, localData.data());
    }

    // make sure all RMA operations are done
    MPI_Barrier(MPI_COMM_WORLD);

    // check
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
            std::cerr << "i = " << i << ", data1 = " << val1 << ", data2 = " << val2 << ", diff = " << std::abs(val1 - val2) << std::endl;
        }
        if (diff < 1e-10) {
            std::cerr << "Put/Get test passed\n";
        } else {
            std::cerr << "Put/Get test failed, diff = " << diff << "\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
}

void testAsyncPutGet(int num_chunks, int num_size) {

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    DistDataCollector dataCollector1(MPI_COMM_WORLD, num_chunks, num_size);
    DistDataCollector dataCollector2(MPI_COMM_WORLD, num_chunks, num_size);

    if (rank == 0) {
        // fill the collected array with known values for testing
        double* data1 = dataCollector1.getCollectedDataPtr();
        for (int i = 0; i < num_chunks; ++i) {
            std::fill(data1 + i * num_size, data1 + (i + 1) * num_size, double(i));
        }
        double* data2 = dataCollector2.getCollectedDataPtr();
        std::fill(data2, data2 + num_chunks*num_size, -1.0);
        
    }

    // make sure all ranks have initialized
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

    // make sure the manager has received all the data
    MPI_Barrier(MPI_COMM_WORLD);

    // check
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
            std::cerr << "i = " << i << ", data1 = " << val1 << ", data2 = " << val2 << ", diff = " << std::abs(val1 - val2) << std::endl;
        }
        if (diff < 1e-10) {
            std::cerr << "Async Put/Get test passed\n";
        } else {
            std::cerr << "Async Put/Get test failed, diff = " << diff << "\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const int num_chunks = size;
    const int num_size = 10; // size of each chunk

    testAsyncPutGet(num_chunks, num_size);

    if (rank == 0) {
        std::cout << "Success\n";
    }
    MPI_Finalize();
    return 0;
}