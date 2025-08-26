#include <Courier.h>
#include <iostream>


void setData(double* data, int size, int rank, double offset) {
    for (int i = 0; i < size; ++i) {
        data[i] = rank * 10 + i + offset; // Fill with some test data
    }
}

void test(const std::vector<int>& dims, 
    int source_worker, int target_worker) {

    // Create an instance of Courier
    Courier courier(MPI_COMM_WORLD);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::size_t numElems = 1;
    for (auto i = 0; i < dims.size(); ++i) {
        numElems *= dims[i];
    }

    double* data = nullptr;

    // Allocate space for source and target data
    if (rank == target_worker || rank == source_worker) {
        data = new double[numElems];
        if (rank == source_worker) {
            setData(data, numElems, rank, 0.0);
        }
    }

    // Expose the data for the source worker
    courier.expose(data, dims);

    // Pull data from the source worker

    courier.get(0, source_worker);

    // Barrier to ensure updates are visible before printing
    MPI_Barrier(MPI_COMM_WORLD);


    if (rank == target_worker) {
        std::cout << "Data on " << rank << " after get from process " << source_worker << ": ";
        for (std::size_t i = 0; i < numElems; ++i) {
            std::cout << data[i] << " ";
        }
        std::cout << std::endl;
    }

    // // Push data to the target worker

    // if (rank == source_worker) {
    //     setData(data, numElems, rank, 100.0); // Change data before pushing
    //     std::cout << "Data on " << rank << " before pushing data: ";
    //     for (std::size_t i = 0; i < numElems; ++i) {
    //         std::cout << data[i] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    
    // courier.put(0, target_worker);

    // if (rank == target_worker) {
    //     std::cout << "Data on " << rank << " after data has been pushed: ";
    //     for (std::size_t i = 0; i < numElems; ++i) {
    //         std::cout << data[i] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    courier.free();
    if (data) {
        delete[] data;
    }
}



int main(int argc, char* argv[]) {
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);
    int nprocs;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    if (nprocs < 2) {
        std::cerr << "This test requires at least 2 MPI processes." << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // test 1d array
    int source_worker = 0;
    int target_worker = nprocs - 1;
    test({3}, source_worker, target_worker);

    if (rank == 0) {
        std::cout << "Success." << std::endl;
    }

    // Finalize the MPI environment
    MPI_Finalize();
    return 0;
}