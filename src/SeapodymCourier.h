#include <mpi.h>
#include <set>
#include <vector>
#ifndef SEAPODYM_COURIER
#define SEAPODYM_COURIER


using Tuple3 = std::tuple<MPI_Win, double*, std::size_t>;

/**
 * @brief SeapodymCourier class for managing memory exposure and data fetching between MPI processes
 * 
 * This class allows workers to expose their memory to other processes and fetch data from them.
 * It uses MPI windows for memory exposure and communication.
 */
class SeapodymCourier {
    
    private:
    
        // MPI communicator to use for communication
        MPI_Comm comm;

        // Pointer to the data exposed by this worker. This class does not own this pointer, 
        // it is provided by the user. The data is expected to be allocated by the user and 
        // should remain valid for the lifetime of this SeapodymCourier instance.
        double *data;
        int data_size; 

        // MPI window for the exposed data
        MPI_Win win;

        // Local MPI rank
        int local_rank;
    public:

    /**
     * @brief Constructor
     * @param comm MPI communicator to use for communication
     */
    SeapodymCourier(MPI_Comm comm=MPI_COMM_WORLD);

    /**
     * @brief Destructor
     */
    ~SeapodymCourier();

    /**
     * @brief Get the pointer to the exposed data
     * @return Pointer to the exposed data
     */
    double* getDataPtr() const { return this->data;}

    /**
     * @brief Expose the memory to other processes
     * @param data Pointer to the data to be exposed
     * @param data_size Number of elements in the data array
     */
    void expose(double* data, int data_size);

    /**
     * @brief Fetch data from a remote process and store it in the local data array
     * @param source_worker Rank of the target process from which to fetch data
     * @return data
     */
    std::vector<double> fetch(int source_worker);

    /**
     * @brief Free the MPI window and reset the data pointer
     */
    void free() {
        if (this->win != MPI_WIN_NULL) {
            MPI_Win_free(&this->win);
        }
        this->data = nullptr;
        this->data_size = 0;
    }

    // Disable copy and assignment operations
    // to prevent accidental copying of the SeapodymCourier instance
    // This is important because the class manages an MPI window and data pointer
    // which should not be copied or assigned.
    SeapodymCourier(const SeapodymCourier&) = delete; // Disable copy constructor
    SeapodymCourier& operator=(const SeapodymCourier&) = delete; // Disable assignment operator
    SeapodymCourier(SeapodymCourier&& other) noexcept; // Move constructor
    SeapodymCourier& operator=(SeapodymCourier&& other) noexcept; // Move assignment operator
};

#endif // SEAPODYM_COURIER