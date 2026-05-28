#include <mpi.h>
#include <cstddef>
#include <cstring>
#include <stdexcept>

#ifndef DATA_PROVIDER_H
#define DATA_PROVIDER_H

/**
 * @brief DataProvider is a class that provides shared memory data access to any local-node MPI process
 */
class DataProvider {

public:

    /**
    * @brief Constructor
    * @param comm root MPI communicator, the shared memory communicator will be derived from this
    * @param data Pointer to the data to share (only used on originRank)
    * @param n Number of elements in the data array
    * @param originRank Rank on the shared memory communicator that provides the data on each node (default: 0)
    */  
    DataProvider(MPI_Comm comm,
                 const double* data,
                 size_t n,
                 int originRank = 0);

    ~DataProvider();

    /**
     * @brief Get a pointer to the shared data array
     * @return pointer to the shared data array
     */
    const double* getDataPtr() const;

    /**
     * @brief Get the number of elements in the shared data array
     * @return number of elements in the shared data array
     */
    size_t getNumElements() const;

private:
    MPI_Comm comm_;
    MPI_Comm shmcomm_;
    MPI_Win win_;

    double* baseptr_;
    size_t n_;
    int shmRank_;
};

#endif // DATA_PROVIDER_H
