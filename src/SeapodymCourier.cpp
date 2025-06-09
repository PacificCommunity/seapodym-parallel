
#include "SeapodymCourier.h"
#include <sstream>
#include <string>
#include <vector>

SeapodymCourier::SeapodymCourier(MPI_Comm comm) {
    this->comm = comm;
    this->data = nullptr;
    this->data_size = 0;
    this->win = MPI_WIN_NULL;
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
