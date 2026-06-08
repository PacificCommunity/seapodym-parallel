#include "DataProvider.h"

DataProvider::DataProvider(MPI_Comm comm, 
                           const std::vector< std::pair<std::string, std::size_t> >& nameSizePairs)
        : comm_(comm), shmcomm_(MPI_COMM_NULL), shmRank_(-1) {

    // Split to get the shared-memory communicator
    MPI_Comm_split_type(
        this->comm_,
        MPI_COMM_TYPE_SHARED,
        0,
        MPI_INFO_NULL,
        &this->shmcomm_);

    MPI_Comm_rank(this->shmcomm_, &this->shmRank_);

    for (const auto& ns : nameSizePairs) {

        const std::string& name = ns.first;
        std::size_t size = ns.second;

        MPI_Aint bytes = 0;
        // Only origin rank allocates memory
        if (this->shmRank_ == 0) { // local rank 0 stores the data
            bytes = static_cast<MPI_Aint>(size) * sizeof(double);
        }

        double* baseptr = nullptr;
        MPI_Win win = MPI_WIN_NULL;

        // Allocate shared memory window for this array
        MPI_Win_allocate_shared(bytes, sizeof(double), MPI_INFO_NULL, this->shmcomm_, &baseptr, &win);

        // Everyone queries origin memory
        MPI_Aint size_mpi;
        int disp_unit;

        MPI_Win_shared_query(win,
        0, // query rank 0's allocation
        &size_mpi,
        &disp_unit,
        &baseptr);

        // Store the base pointer, number of elements, and window in the data map
        this->data_[name] = std::make_tuple(baseptr, size, win);
    }
  
}

DataProvider::~DataProvider() {

    for (auto& [name, tuple] : this->data_) {
        auto& [baseptr, n, win] = tuple;
        if (win != MPI_WIN_NULL) {
            MPI_Win_free(&win);
        }
    }

    if (this->shmcomm_ != MPI_COMM_NULL)
        MPI_Comm_free(&this->shmcomm_);
}
