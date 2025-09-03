
#include "DistDataCollector.h"
#include <sstream>
#include <string>
#include <vector>
#include <limits>

DistDataCollector::DistDataCollector(MPI_Comm comm, int numChunks, int numSize) {
    this->comm = comm;
    int rank, nproc;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);

    this->numChunks = numChunks;
    this->numSize = numSize;

    // Allocate and create the window, zero size on other ranks than 0
    MPI_Aint winSize = (rank == 0) ? (numChunks * numSize * sizeof(double)) : 0;
    MPI_Win_allocate(winSize, sizeof(double), MPI_INFO_NULL,
                        comm, &this->collectedData, &this->win);
}

DistDataCollector::~DistDataCollector() {
    this->free();
}

void
DistDataCollector::put(int chunkId, const double* data) {

    // Synchronize before RMA operation. Each rank will write
    // disjoint pieces of data, so we can use shared locks
    MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);

    // Put local_data into the appropriate slice on rank 0
    MPI_Put(data, this->numSize, MPI_DOUBLE,
                0, chunkId * this->numSize, this->numSize, MPI_DOUBLE, this->win);

    // Synchronize after RMA operations
    MPI_Win_unlock(0, this-> win);
}

std::vector<double>
DistDataCollector::get(int chunkId) {

    // Synchronize before RMA operation. Each rank will read
    // disjoint pieces of data, so we can use shared locks
    MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);

    // Get the appropriate slice from rank 0
    std::vector<double> data(this->numSize);
    MPI_Get(data.data(), this->numSize, MPI_DOUBLE,
                0, chunkId * this->numSize, this->numSize, MPI_DOUBLE, this->win);

    // Synchronize after RMA operations
    MPI_Win_unlock(0, this-> win);

    return data;
}
