
#include "SeapodymCourier.h"
#include <sstream>
#include <string>
#include <vector>

SeapodymCourier::SeapodymCourier(MPI_Comm comm) {
    this->comm = comm;
    this->data = nullptr;
    this->data_size = 0;
    this->win = MPI_WIN_NULL;
    this->winRecv = MPI_WIN_NULL;
    MPI_Comm_rank(comm, &this->local_rank);
}

SeapodymCourier::~SeapodymCourier() {
    this->free();
}

void 
SeapodymCourier::expose(double* data, int data_size) {
    this->data = data;
    this->data_size = data_size;
    // Create an MPI window to expose the data
    if (this->win != MPI_WIN_NULL) {
        MPI_Win_free(&this->win);
    }
    MPI_Win_create(data, data_size * sizeof(double), sizeof(double), MPI_INFO_NULL, this->comm, &this->win);

    // Create window to reveive the result of MPI_Accumulate
    this->dataRecv.resize(data_size);
    MPI_Win_create(this->dataRecv.data(), data_size * sizeof(double), sizeof(double), MPI_INFO_NULL, this->comm, &this->winRecv);
}

std::vector<double>
SeapodymCourier::fetch(int source_rank) {

    if (this->local_rank == source_rank) {
        // If the source rank is the same as the local rank, we can just copy the data
        // This avoids unnecessary MPI calls when fetching from self
        return std::vector<double>(this->data, this->data + this->data_size);
    }

    std::vector<double> res(this->data_size);

    // Ensure the window is ready for access
    // MPI_LOCK_SHARED allows multiple processes to read from the window simultaneously
    // This is useful when multiple processes need to fetch data from the same source
    // Note: MPI_LOCK_SHARED is used here to allow concurrent reads, but it can be replaced with MPI_LOCK_EXCLUSIVE if exclusive access is needed
    MPI_Win_lock(MPI_LOCK_SHARED, source_rank, MPI_MODE_NOCHECK, this->win);
    
    // Fetch the data from the remote process
    MPI_Get(res.data(), this->data_size, MPI_DOUBLE, source_rank, 0, this->data_size, MPI_DOUBLE, this->win);
    
    // Complete the access to the window
    MPI_Win_unlock(source_rank, this->win);

    return res;
}

std::vector<double>
SeapodymCourier::accumulate(int targetWorker) {
    
    // Ensure the window is ready for access
    // Possible values are MPI_MODE_NOCHECK, MPI_MODE_NOSTORE, MPI_MODE_NOPUT, MPI_MODE_NOSUCCEED
    // MPI_MODE_NOPRECEDE:  No RMA calls before this point can access the window
    MPI_Win_fence(MPI_MODE_NOPRECEDE, this->winRecv);

    // Need to reset the buffer to zero, otherwise it will add to the exisiting values
    std::fill(this->dataRecv.begin(), this->dataRecv.end(), 0.0);

    // The result of the reduction operation will be in this->dataRecv
    MPI_Accumulate(this->data, this->data_size, MPI_DOUBLE, targetWorker, 0, this->data_size, MPI_DOUBLE, MPI_SUM, this->winRecv);

    // Complete the access to the window, no RMA calls after this point
    MPI_Win_fence(MPI_MODE_NOSUCCEED, this->winRecv);

    return this->dataRecv;
}
