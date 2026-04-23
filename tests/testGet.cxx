#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdlib>

void init_data(std::vector<double>& data) {
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = static_cast<double>(i);
    }
}

void check_data(const std::vector<double>& buf, int numData, int rank) {
    if (rank == 0) return;

    int offset = (rank - 1) * numData;
    for (int i = 0; i < numData; i++) {
        if (buf[i] != static_cast<double>(offset + i)) {
            std::cout << "Rank " << rank << ": data validation FAILED\n";
            return;
        }
    }
    std::cout << "check_data succeeded.\n";
}

double test_fence(MPI_Win win, std::vector<double>& localBuf,
                  int numData, int rank) {
    double t0 = MPI_Wtime();

    MPI_Win_fence(0, win);

    if (rank != 0) {
        int offset = (rank - 1) * numData;
        MPI_Get(localBuf.data(), numData, MPI_DOUBLE,
                0, offset, numData, MPI_DOUBLE, win);
    }

    MPI_Win_fence(0, win);

    return MPI_Wtime() - t0;
}

double test_lock_all(MPI_Win win, std::vector<double>& localBuf,
                     int numData, int rank) {
    double t0 = MPI_Wtime();

    MPI_Win_lock_all(0, win);

    if (rank != 0) {
        int offset = (rank - 1) * numData;
        MPI_Get(localBuf.data(), numData, MPI_DOUBLE,
                0, offset, numData, MPI_DOUBLE, win);
    }

    MPI_Win_flush(0, win);

    MPI_Win_unlock_all(win);

    return MPI_Wtime() - t0;
}

double test_lock(MPI_Win win, std::vector<double>& localBuf,
                 int numData, int rank) {
    double t0 = MPI_Wtime();

    if (rank != 0) {
        int offset = (rank - 1) * numData;

        MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, win);

        MPI_Get(localBuf.data(), numData, MPI_DOUBLE,
                0, offset, numData, MPI_DOUBLE, win);

        MPI_Win_flush(0, win);

        MPI_Win_unlock(0, win);
    }

    return MPI_Wtime() - t0;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0) {
            std::cout << "Run with at least 2 ranks\n";
        }
        MPI_Finalize();
        return 0;
    }

    int numWorkers = size - 1;
    int numData = 100000; // adjust as needed
    int totalSize = numWorkers * numData;

    std::vector<double> winData;
    std::vector<double> localBuf(numData);

    if (rank == 0) {
        winData.resize(totalSize);
        init_data(winData);
    }

    MPI_Win win;
    MPI_Win_create(
        rank == 0 ? winData.data() : nullptr,
        (rank == 0 ? totalSize : 0) * sizeof(double),
        sizeof(double),
        MPI_INFO_NULL,
        MPI_COMM_WORLD,
        &win
    );

    MPI_Barrier(MPI_COMM_WORLD);

    double t_fence = test_fence(win, localBuf, numData, rank);
    MPI_Barrier(MPI_COMM_WORLD);
    check_data(localBuf, numData, rank);

    double t_lock_all = test_lock_all(win, localBuf, numData, rank);
    MPI_Barrier(MPI_COMM_WORLD);
    check_data(localBuf, numData, rank);

    double t_lock = test_lock(win, localBuf, numData, rank);
    MPI_Barrier(MPI_COMM_WORLD);
    check_data(localBuf, numData, rank);

    double max_fence, max_lock_all, max_lock;

    MPI_Reduce(&t_fence, &max_fence, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_lock_all, &max_lock_all, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_lock, &max_lock, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "\n=== MPI RMA Get Benchmark ===\n";
        std::cout << "numWorkers = " << numWorkers
                  << ", numData = " << numData << "\n";
        std::cout << "Fence time     : " << max_fence << " s\n";
        std::cout << "Lock_all time  : " << max_lock_all << " s\n";
        std::cout << "Lock time      : " << max_lock << " s\n";
    }

    MPI_Win_free(&win);

    MPI_Finalize();
    return 0;
}
