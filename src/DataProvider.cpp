#include "DataProvider.h"

DataProvider::DataProvider(MPI_Comm comm, size_t n)
        : comm_(comm), win_(MPI_WIN_NULL), baseptr_(nullptr), n_(0) {

    // Split to shared-memory communicator
    MPI_Comm_split_type(
        this->comm_,
        MPI_COMM_TYPE_SHARED,
        0,
        MPI_INFO_NULL,
        &this->shmcomm_);

    MPI_Comm_rank(this->shmcomm_, &this->shmRank_);

    // number of element in the shared array
    this->n_ = n;

    MPI_Aint bytes = 0;

    // Only origin rank allocates memory
    if (this->shmRank_ == 0) { // local rank that stores the data is 0 
        bytes = static_cast<MPI_Aint>(this->n_) * sizeof(double);
    }

    // Allocate shared memory window
    MPI_Win_allocate_shared(
        bytes,
        sizeof(double),
        MPI_INFO_NULL,
        this->shmcomm_,
        &this->baseptr_,
        &this->win_);

    // Everyone queries origin memory
    MPI_Aint size;
    int disp_unit;

    MPI_Win_shared_query(
        this->win_,
        0, // query rank 0's allocation
        &size,
        &disp_unit,
        &this->baseptr_);
}

DataProvider::~DataProvider() {
    if (this->win_ != MPI_WIN_NULL)
        MPI_Win_free(&this->win_);

    if (this->shmcomm_ != MPI_COMM_NULL)
        MPI_Comm_free(&this->shmcomm_);
}
