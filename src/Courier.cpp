
#include "Courier.h"
#include <sstream>
#include <string>
#include <vector>

Courier::Courier(MPI_Comm comm) {
    this->comm = comm;
    this->data = nullptr;
    this->dims.clear();
    this->numElems = 0;
    this->win = MPI_WIN_NULL;
}

Courier::~Courier() {
    this->free();
}

void 
Courier::expose(double* data, const std::vector<int>& dims) {
    this->data = data;
    this->dims = dims;
    // Create an MPI window to expose the data
    if (this->win != MPI_WIN_NULL) {
        MPI_Win_free(&this->win);
    }
    this->numElems = 1;
    for (auto i = 0; i < this->dims.size(); ++i) {
        this->numElems *= this->dims[i];
    }
    MPI_Win_create(data, this->numElems * sizeof(double), sizeof(double), MPI_INFO_NULL, this->comm, &this->win);
}

void
Courier::get(int source_worker) {

    // Ensure the window is ready for access
    // MPI_LOCK_SHARED allows multiple processes to read from the window simultaneously
    // This is useful when multiple processes need to fetch data from the same source
    // Note: MPI_LOCK_SHARED is used here to allow concurrent reads, but it can be replaced with MPI_LOCK_EXCLUSIVE if exclusive access is needed
    MPI_Win_lock(MPI_LOCK_SHARED, source_worker, MPI_MODE_NOCHECK, this->win);
    
    // Fetch the data from the remote process
    MPI_Get(this->data, this->numElems, MPI_DOUBLE, source_worker, 0, this->numElems, MPI_DOUBLE, this->win);

    // Complete the access to the window
    MPI_Win_unlock(source_worker, this->win);
}

void
Courier::put(int target_worker) {

    // Ensure the window is ready for access
    // MPI_LOCK_SHARED allows multiple processes to read from the window simultaneously
    // This is useful when multiple processes need to fetch data from the same source
    // Note: MPI_LOCK_SHARED is used here to allow concurrent reads, but it can be replaced 
    // with MPI_LOCK_EXCLUSIVE if exclusive access is needed
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target_worker, MPI_MODE_NOCHECK, this->win);

    // Fetch the data from the remote process
    MPI_Put(this->data, this->numElems, MPI_DOUBLE, target_worker, 0, this->numElems, MPI_DOUBLE, this->win);

    // Complete the access to the window
    MPI_Win_unlock(target_worker, this->win);
}
