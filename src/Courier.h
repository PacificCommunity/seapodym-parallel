#include <mpi.h>
#include <set>
#include <vector>
#ifndef COURIER
#define COURIER


using Tuple3 = std::tuple<MPI_Win, double*, std::size_t>;

/**
 * @brief Courier is a class getting and putting data between MPI processes
 */
class Courier {
    
    private:
    
        // MPI communicator to use for communication
        MPI_Comm comm;

        // Pointer to the data exposed by this worker. This class does not own this pointer, 
        // it is provided by the user. The data is expected to be allocated by the user and 
        // should remain valid for the lifetime of this Courier instance.
        double *data;
        // dimensions of the data
        std::vector<int> dims;
        // number of elements
        std::size_t numElems;

        // MPI window for the exposed data
        MPI_Win win;

        public:

    /**
     * @brief Constructor
     * @param comm MPI communicator to use for communication
     */
    Courier(MPI_Comm comm=MPI_COMM_WORLD);

    /**
     * @brief Destructor
     */
    ~Courier();

    /**
     * @brief Get the pointer to the exposed data
     * @return Pointer to the exposed data
     */
    double* getDataPtr() const {return this->data;}

    /**
     * @brief Get the dimensions of the data
     * @return Vector containing the dimensions of the data
     */
    std::vector<int> getDataDims() const {return this->dims;}

    /**
     * @brief Expose the memory to other processes
     * @param data Pointer to the data to be exposed, can be nullptr
     * @param dims Dimensions of the data array
     * @note this should be executed on the processes
     */
    void expose(double* data, const std::vector<int>& dims);

    /**
     * @brief Get the data from a remote process and store it in the local data array
     * @param offset Displacement (in number of elements) in the target data array
     * @param source_worker Rank of the process from which to get data
     * @note this should be executed on the target process
     */
    void get(int offset, int source_worker);

    /**
     * @brief Put the data to a remote process and store it in the local data array
     * @param offset Displacement (in number of elements) in the target data array
     * @param target_worker Rank of the process that will receive the data
     * @note this should be executed on the source process
     */
    void put(int offset, int target_worker);

    /**
     * @brief Free the MPI window and reset the data pointer
     */
    void free() {
        if (this->win != MPI_WIN_NULL) {
            MPI_Win_free(&this->win);
        }
        // this object does not own the data
        this->data = nullptr;
        this->dims.clear();
    }

    // Disable copy and assignment operations
    // to prevent accidental copying of the Courier instance
    // This is important because the class manages an MPI window and data pointer
    // which should not be copied or assigned.
    Courier(const Courier&) = delete; // Disable copy constructor
    Courier& operator=(const Courier&) = delete; // Disable assignment operator
    Courier(Courier&& other) noexcept; // Move constructor
    Courier& operator=(Courier&& other) noexcept; // Move assignment operator
};

#endif // COURIER
