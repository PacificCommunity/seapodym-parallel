#include "DataProvider.h"

DataProvider::DataProvider(MPI_Comm comm, const double* data, size_t n, int originRank)
        : comm_(comm), win_(MPI_WIN_NULL), baseptr_(nullptr), n_(n) {

    // Split to shared-memory communicator
    MPI_Comm_split_type(
        comm_,
        MPI_COMM_TYPE_SHARED,
        0,
        MPI_INFO_NULL,
        &shmcomm_);

    MPI_Comm_rank(shmcomm_, &shmRank_);

    MPI_Aint bytes = 0;

    // Only origin rank allocates memory
    if (shmRank_ == originRank) {
        bytes = static_cast<MPI_Aint>(n_) * sizeof(double);
    }

    // Allocate shared memory window
    MPI_Win_allocate_shared(
        bytes,
        sizeof(double),
        MPI_INFO_NULL,
        shmcomm_,
        &baseptr_,
        &win_);

    // Everyone queries origin memory
    MPI_Aint size;
    int disp_unit;

    MPI_Win_shared_query(
        win_,
        originRank,
        &size,
        &disp_unit,
        &baseptr_);

    // Initialize on origin rank
    if (shmRank_ == originRank)
    {
        std::memcpy(baseptr_, data, n_ * sizeof(double));
    }

    // Ensure visibility of initialization
    MPI_Barrier(shmcomm_);
    MPI_Win_sync(win_);
    MPI_Barrier(shmcomm_);
}

DataProvider::~DataProvider() {
    if (win_ != MPI_WIN_NULL)
        MPI_Win_free(&win_);

    if (shmcomm_ != MPI_COMM_NULL)
        MPI_Comm_free(&shmcomm_);
}

const double* 
DataProvider::getDataPtr() const{
    return baseptr_;
}

size_t
DataProvider::getNumElements() const {
    return n_;
}
