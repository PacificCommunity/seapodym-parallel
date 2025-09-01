
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

    if (rank == 0) {
        // allocate the collected data on rank 0
        this->collectedData.resize(numChunks * numSize, std::numeric_limits<double>::infinity());
    }

    // set up the MPI window. Only rank 0 requests storage
    void* basePtr = this->collectedData.empty() ? nullptr : this->collectedData.data();
    MPI_Aint winSize = this->collectedData.size() * sizeof(double);

    // This fails with OpenMPI if one only has a single rank (should not)
    MPI_Win_create(basePtr, winSize, 
        sizeof(double), MPI_INFO_NULL, comm, &this->win);
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