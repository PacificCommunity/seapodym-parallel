
#include "DistDataCollector.h"
#include <sstream>
#include <string>
#include <vector>

DistDataCollector::DistDataCollector(MPI_Comm comm, int numChunks, int numSize) {
    this->comm = comm;
    int rank, nproc;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);

    this->numChunks = numChunks;
    this->numSize = numSize;
    // allocate the data on rank 0
    if (rank == 0) {
        this->collectedData.resize(numChunks * numSize);
    } else {
        this->collectedData.clear();
    }

    // set up the MPI window
    MPI_Win_create(this->collectedData.data(), 
        this->collectedData.size() * sizeof(double), 
        sizeof(double), MPI_INFO_NULL, comm, &this->win);
}

DistDataCollector::~DistDataCollector() {
    this->free();
}

void
DistDataCollector::inject(int chunkId, const double* data) {

    // Synchronize before RMA operations
    MPI_Win_fence(0, this->win);

    // Put local_data into the appropriate slice on rank 0
    MPI_Put(data, this->numSize, MPI_DOUBLE,
                0, chunkId * this->numSize, this->numSize, MPI_DOUBLE, this->win);

    // Synchronize after RMA operations
    MPI_Win_fence(0, this-> win);
}
