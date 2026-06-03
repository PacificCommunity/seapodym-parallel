#include <mpi.h>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <string>
#include <unordered_map>
#include <tuple>

#ifndef DATA_PROVIDER_H
#define DATA_PROVIDER_H

/**
 * @brief DataProvider is a class that provides shared memory data access to any local-node MPI process
 */
class DataProvider {

public:

    DataProvider(const DataProvider&) = delete;
    DataProvider& operator=(const DataProvider&) = delete;

    /**
    * @brief Constructor
    * @param comm root MPI communicator, the shared memory communicator will be derived from this
    * @param nameSizePairs vector of pairs containing shared array names and their sizes
    */  
    DataProvider(MPI_Comm comm, const std::vector<std::pair<std::string, std::size_t> >& nameSizePairs);

    ~DataProvider();

    /** 
     * @brief Get the rank of the calling process in the shared memory communicator
     * @return rank of the calling process in the shared memory communicator
     */
    int getShmRank() const { return shmRank_; }

    /**
     * @brief Check if the calling process is the root of the shared memory communicator (i.e., the one that provides the data)
     * @return true if the calling process is the root of the shared memory communicator, false otherwise
     */
    bool isShmRoot() const { return shmRank_ == 0; }

    /**
     * @brief Get a pointer to the shared data array
     * @return pointer to the shared data array
     * @note the returned pointer is valid only within the same shared memory node, and should not be used for MPI communication 
     * across different nodes. Additionally, the caller should ensure that the shared data array is properly synchronized 
     * (e.g., using MPI_Barrier) before accessing the data to ensure visibility of the data updates across all ranks on the 
     * same node.
     */
    double* getDataPtr(const std::string& name) const {
        auto it = this->data_.find(name);
        return it != this->data_.end() ? std::get<0>(it->second) : nullptr;
    }

    /**
     * @brief Get the number of elements in the shared data array
     * @return number of elements in the shared data array
     */
    size_t getNumElements(const std::string& name) const {
        auto it = this->data_.find(name);
        return it != this->data_.end() ? std::get<1>(it->second) : 0;
    }

    /**
     * Get the MPI shm communicator
     * @return the MPI shm communicator
     */
    MPI_Comm getShmComm() const {
        return this->shmcomm_;
    }

private:
    MPI_Comm comm_;
    MPI_Comm shmcomm_;
    std::unordered_map< std::string, std::tuple<double*, std::size_t, MPI_Win> > data_;
    int shmRank_;
};

#endif // DATA_PROVIDER_H
