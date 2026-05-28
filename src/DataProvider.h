#include <mpi.h>
#include <cstddef>
#include <cstring>
#include <stdexcept>

#ifndef DATA_PROVIDER_H
#define DATA_PROVIDER_H

class DataProvider {

public:
    DataProvider(MPI_Comm comm,
                 const double* data,
                 size_t n,
                 int originRank = 0);

    ~DataProvider();

    // Read-only access to shared data
    const double* getDataPtr() const;

    size_t size() const;

private:
    MPI_Comm comm_;
    MPI_Comm shmcomm_;
    MPI_Win win_;

    double* baseptr_;
    size_t n_;
    int shmRank_;
};

#endif // DATA_PROVIDER_H
