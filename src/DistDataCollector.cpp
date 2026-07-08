
#include "DistDataCollector.h"
#include <sstream>
#include <string>
#include <vector>
#include <cmath> // std::isnan()

DistDataCollector::DistDataCollector(MPI_Comm comm, int numChunks, int numSize, int rootRank) {
    this->comm = comm;
    this->rootRank = rootRank;
    int rank, nproc;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);

    this->numChunks = numChunks;
    this->numSize = numSize;

    // Allocate and create the window, zero size on ranks other than rootRank
    MPI_Aint winSize = (rank == rootRank) ? (numChunks * numSize * sizeof(double)) : 0;
    MPI_Win_allocate(winSize, sizeof(double), MPI_INFO_NULL,
                        comm, &this->collectedData, &this->win);

    // Initialize the collected data with bad values
    if (rank == rootRank) {
        std::fill(this->collectedData, this->collectedData + (numChunks * numSize), BAD_VALUE);
    }

    // MPI_Win_allocate is collective and returns on every rank once the
    // memory is allocated, but it does NOT wait for rootRank's subsequent,
    // purely-local fill() above to finish. For a large window (~GBs), this can
    // take several seconds. Without this barrier, any other rank would be free to start
    // issuing put()/get() calls into the window as soon as its own
    // MPI_Win_allocate returns. The sysmptom would be NaNs in the rootRank's window
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
    MPI_Win_lock(MPI_LOCK_SHARED, this->rootRank, 0, this->win);

    // Put local_data into the appropriate slice on rootRank
    MPI_Put(data, this->numSize, MPI_DOUBLE,
                this->rootRank, chunkId * this->numSize, this->numSize, MPI_DOUBLE, this->win);
    MPI_Win_flush(this->rootRank, this->win);

    // Synchronize after RMA operations
    MPI_Win_unlock(this->rootRank, this->win);
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
    MPI_Win_lock(MPI_LOCK_SHARED, this->rootRank, 0, this->win);

    // Get the appropriate slice from rootRank
    MPI_Get(buffer, this->numSize, MPI_DOUBLE,
                this->rootRank, chunkId * this->numSize, this->numSize, MPI_DOUBLE, this->win);
    MPI_Win_flush(this->rootRank, this->win);

    // Synchronize after RMA operations
    MPI_Win_unlock(this->rootRank, this->win);
}

void
DistDataCollector::accumulate(int chunkId, const double* data) {

    // Shared lock: concurrent MPI_Accumulate calls with the same op (MPI_SUM)
    // from different origins are safe under shared locks per the MPI standard.
    MPI_Win_lock(MPI_LOCK_SHARED, this->rootRank, 0, this->win);

    MPI_Accumulate(data, this->numSize, MPI_DOUBLE,
                   this->rootRank, chunkId * this->numSize, this->numSize, MPI_DOUBLE,
                   MPI_SUM, this->win);
    MPI_Win_flush(this->rootRank, this->win);

    MPI_Win_unlock(this->rootRank, this->win);
}

