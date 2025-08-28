
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

    if (rank == 0) {
        // allocate the collected data on rank 0
        this->collectedData.resize(numChunks * numSize);
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
DistDataCollector::inject(int chunkId, const double* data) {

    // Synchronize before RMA operations. Each rank will write contribute
    // disjoint pieces of data, so we can use shared locks
    MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);

    // Put local_data into the appropriate slice on rank 0
    MPI_Put(data, this->numSize, MPI_DOUBLE,
                0, chunkId * this->numSize, this->numSize, MPI_DOUBLE, this->win);

    // Synchronize after RMA operations
    MPI_Win_unlock(0, this-> win);
}
