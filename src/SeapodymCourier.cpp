
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
    if (this->win != MPI_WIN_NULL) {
        MPI_Win_free(&this->win);
    }
    // The data pointer is owned by the caller, so we do not free it here.
    this->data = nullptr;
    this->data_size = 0;
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

void 
SeapodymCourier::fetch(int target_rank) {

    // Ensure the window is ready for access
    // MPI_MODE_NOPUT tells MPI that the calling process will not do any MPI_Put operations before the next fence
    // MPI_MODE_NOPRECEDE ensures that the fence is not preceded by any other operations
    MPI_Win_fence(MPI_MODE_NOPUT | MPI_MODE_NOPRECEDE, this->win);
    
    // Fetch the data from the remote process
    MPI_Get(this->data, this->data_size, MPI_DOUBLE, target_rank, 0, this->data_size, MPI_DOUBLE, this->win);
    
    // Complete the access to the window
    // MPI_MODE_NOSUCCEED tells MPI that the calling process does not expect any successful completion of operations
    MPI_Win_fence(MPI_MODE_NOSUCCEED, this->win);
}

void 
SeapodymCourier::accumulate(const std::set<int>& source_workers, int target_worker) {
    // Ensure the window is ready for access
    MPI_Win_fence(MPI_MODE_NOPUT | MPI_MODE_NOPRECEDE, this->win);

    if (source_workers.count(this->local_rank) == 1) {
        // If the local worker is in the source workers, we can accumulate data
        // This worker's data will be accumulated into the target worker's data
        MPI_Accumulate(this->data, this->data_size, MPI_DOUBLE, target_worker, 0, this->data_size, MPI_DOUBLE, MPI_SUM, this->win);
    }
    
    // Complete the access to the window
    MPI_Win_fence(MPI_MODE_NOSUCCEED, this->win);
}
