
#include "DistDataCollector.h"
#include <sstream>
#include <string>
#include <vector>
#include <cmath> // std::isnan()
#include <iostream>

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

    // Initialize the collected data with bad values
    if (rank == 0) {
        std::fill(this->collectedData, this->collectedData + (numChunks * numSize), BAD_VALUE);
    }

    // Make sure all processes completed the creation of the MPI Window
    MPI_Barrier(comm); 
}

DistDataCollector::~DistDataCollector() {
    
    if (this->win != MPI_WIN_NULL) {
        MPI_Win_free(&this->win);
        this->win = MPI_WIN_NULL;
    }
    // No need to free the data, MPI_Win_free will free the pointer
    //MPI_Free_mem(this->collectedData);
}

void 
DistDataCollector::fence() {
    MPI_Win_fence(0, this->win);
}

void
DistDataCollector::put(int chunkId, const double* data) {

    // Synchronize before RMA operation. Each rank will write
    // disjoint pieces of data, so we can use shared locks
    MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, this->win);

    // Put local_data into the appropriate slice on rank 0
    MPI_Put(data, this->numSize, MPI_DOUBLE,
                0, chunkId * this->numSize, this->numSize, MPI_DOUBLE, this->win);
    MPI_Win_flush(0, this->win);

    // Synchronize after RMA operations
    MPI_Win_unlock(0, this->win);
}

std::vector<double>
DistDataCollector::get(int chunkId) {

    std::vector<double> data(this->numSize);
    this->get(chunkId, data.data());

    return data;
}

void
DistDataCollector::get(int chunkId, double* buffer) {

    // Synchronize before RMA operation. Each rank will read
    // disjoint pieces of data, so we can use shared locks
    MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, this->win);

    // Get the appropriate slice from rank 0
    MPI_Get(buffer, this->numSize, MPI_DOUBLE,
                0, chunkId * this->numSize, this->numSize, MPI_DOUBLE, this->win);
    MPI_Win_flush(0, this->win);

    // Synchronize after RMA operations
    MPI_Win_unlock(0, this->win);
}

