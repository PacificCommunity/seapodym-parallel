#include <mpi.h>
#include <set>
#include <vector>
#ifndef DIST_DATA_COLLECTOR
#define DIST_DATA_COLLECTOR

    
using Tuple3 = std::tuple<MPI_Win, double*, std::size_t>;

/**
 * @brief DistDataCollector is a class that collects data stored on multiple MPI processes
 *                          into a large array stored on rank 0
 */
class DistDataCollector {
    
    private:
    
        // MPI communicator to use for communication
        MPI_Comm comm;

        // number of chunks
        std::size_t numChunks;

        // local size of the data
        std::size_t numSize;

        // the array that collects the data of size numChunks * numSize on rank 0
        double* collectedData;
        
        // MPI window for remote memory access
        MPI_Win win;

        public:

    /**
     * @brief Constructor
     * @param comm MPI communicator to use for communication
     * @param numChunks The number of array slices on rank 0
     * @param numSize The size of each slice
     */
    DistDataCollector(MPI_Comm comm, int numChunks, int numSize);

    /**
     * @brief Destructor
     */
    ~DistDataCollector();

    /**
     * @brief Put the local data into the collected array 
     * @param chunkId Leading index in the collected array
     * @param data Pointer to the local data to inject  
     * @note this should be executed on the source process, typically on the worker
     */
    void put(int chunkId, const double* data);

    /**
     * @brief Get a slice of the remote, collected array to the local worker
     * @param chunkId Leading index in the collected array
     * @return data array 
     * @note this should be executed on the source process, typically on the worker
     */
    std::vector<double> get(int chunkId);

    /**
     * Get the pointer to the collected data
     * @return pointer
     * @note this returns a null pointer on ranks other than 0
     */
    double* getCollectedDataPtr() {
        return this->collectedData;
    }

    /**
     * @brief Free the MPI window and empty the collected data
     */
    void free() {
        if (this->win != MPI_WIN_NULL) {
            MPI_Win_free(&this->win);
        }
        // No need to free the data, MPI_Win_free will free the pointer
        //MPI_Free_mem(this->collectedData);
    }

    // Disable copy and assignment operations
    // to prevent accidental copying of the DistDataCollector instance
    // This is important because the class manages an MPI window and data pointer
    // which should not be copied or assigned.
    DistDataCollector(const DistDataCollector&) = delete; // Disable copy constructor
    DistDataCollector& operator=(const DistDataCollector&) = delete; // Disable assignment operator
    DistDataCollector(DistDataCollector&& other) noexcept; // Move constructor
    DistDataCollector& operator=(DistDataCollector&& other) noexcept; // Move assignment operator
};

#endif // DIST_DATA_COLLECTOR
