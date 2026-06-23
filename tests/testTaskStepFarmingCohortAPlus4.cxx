/**
 * testTaskStepFarmingCohortAPlus4.cxx
 *
 * Simplest correct A+ implementation: uses MPI_Accumulate (via
 * DistDataCollector::accumulate) so feeder cohorts add their output directly
 * into the running A+ total in one RMA call.  No A+ worker rank, no split
 * communicator, and no ping-pong handshake are needed.
 *
 * Design
 * ------
 *   aplusCollect  – 1 chunk of numData doubles, rootRank = 0 (manager).
 *                   Initialised to zero before farming begins.
 *
 *   Feeder cohort (task_id 0..nt-2) at step na-1:
 *     1. aplusCollect->accumulate(0, localData)
 *           → MPI_Accumulate with MPI_SUM under a shared lock.
 *             Blocks until the addition is visible at rank 0.
 *     2. MPI_Send END_TASK_TAG to manager.
 *           → Manager marks feeder done and unblocks new cohorts.
 *
 *   Ordering guarantee (no extra messaging required):
 *     accumulate() returns only after the data is visible at rank 0  →
 *     END_TASK_TAG is sent  →  manager assigns a new cohort  →
 *     new cohort calls aplusCollect->get(0, …)  →  sees the updated total.
 *
 *   New cohort (task_id >= na):
 *     Reads aplusCollect chunk 0 as part of its initial conditions.
 *
 * Requires >= 2 MPI ranks (1 manager + 1 worker).
 *
 * Expected checksums (na=5, nt=10, nd=100000, age_mature=0 or 1):
 *   dataCollect checksum                      : 32500000
 *   aplusCollect accumulator (chunk 0) checksum: 3600000   (= 36 * 100000)
 */

#include <mpi.h>
#include <iostream>
#include <functional>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <random>
#include <CmdLineArgParser.h>
#include "TaskStepManager.h"
#include "TaskStepWorker.h"
#include "SeapodymCohortDependencyAnalyzer.h"
#include "DistDataCollector.h"
#undef NDEBUG
#include <cassert>

/**
 * Chunk index for a (task_id, step) pair in the main data collector.
 */
int inline getChunkId(int task_id, int step, int na) {
    int row = task_id + step - na + 1;
    int col = task_id % na;
    return row * na + col;
}

/**
 * Task function executed by every worker for every assigned cohort.
 *
 * @param comm        MPI_COMM_WORLD – used for END_TASK_TAG to manager and
 *                    for all RMA operations on dataCollector / aplusCollect.
 * @param numAgeGroups na
 * @param numTimeSteps nt
 * @param numData      number of doubles per chunk
 * @param dataCollector  main result collector (rootRank = 0)
 * @param aplusCollect   A+ accumulator, 1 chunk (rootRank = 0)
 * @param dependencyMap  cohort dependency graph
 */
void inline
taskFunction(int task_id, int stepBeg, int stepEnd, MPI_Comm comm,
    int ms, int init_milliseconds, int numAgeGroups, int numTimeSteps, int numData,
    DistDataCollector* dataCollector,
    DistDataCollector* aplusCollect,
    std::map<int, std::set<std::array<int, 2>>>* dependencyMap,
    std::mt19937* rng, std::gamma_distribution<double>* dist) {

    const int endTaskTag = 1; // END_TASK_TAG

    std::vector<double> localData(numData, 0.0);
    std::vector<double> data(numData, 0.0);

    // ------------------------------------------------------------------
    // Gather initial conditions from normal-cohort dependencies.
    // ------------------------------------------------------------------
    for (const auto& [task_id2, step] : (*dependencyMap)[task_id]) {
        int chunk_id = getChunkId(task_id2, step, numAgeGroups);
        dataCollector->get(chunk_id, data.data());

        if (!data.empty() && data.back() == dataCollector->BAD_VALUE)
            MPI_Abort(comm, 1);

        std::transform(data.begin(), data.end(),
                       localData.begin(), localData.begin(), std::plus<double>());

    }

    // New cohorts (task_id >= na) also read the A+ accumulator.
    // By the time the manager assigns this cohort, every feeder that this
    // cohort depends on has already completed its accumulate() + END_TASK_TAG,
    // so chunk 0 of aplusCollect is guaranteed up to date.
    if (task_id >= numAgeGroups) {
        aplusCollect->get(0, data.data());
        std::transform(data.begin(), data.end(),
                       localData.begin(), localData.begin(), std::plus<double>());
    }

    // pretend to spend time initialising
    std::this_thread::sleep_for( std::chrono::milliseconds(init_milliseconds) );

    // ------------------------------------------------------------------
    // Step through the cohort's time range.
    // ------------------------------------------------------------------
    for (auto step = stepBeg; step < stepEnd; ++step) {

        int tsleep = static_cast<int>(std::round((*dist)(*rng)));
        std::this_thread::sleep_for(std::chrono::milliseconds(tsleep));

        // Simulate computation: fill with task_id.
        std::fill(localData.begin(), localData.end(), double(task_id));

        // Publish to main data collector.
        int chunk_id = getChunkId(task_id, step, numAgeGroups);
        dataCollector->put(chunk_id, localData.data());

        // Feeder cohorts accumulate into the A+ buffer at step na-1, then
        // simulate A+ processing work with the same sleep distribution as a
        // normal step.  The sleep happens before END_TASK_TAG so the manager
        // only unblocks downstream cohorts once both accumulation and A+ work
        // are complete.
        if (step == numAgeGroups - 1 && task_id <= numTimeSteps - 2) {
            aplusCollect->accumulate(0, localData.data());
            int tsleep_aplus = static_cast<int>(std::round((*dist)(*rng)));
            std::this_thread::sleep_for(std::chrono::milliseconds(tsleep_aplus));
        }

        // Notify manager that this step is complete.
        int output[3] = {task_id, step, task_id};
        MPI_Send(output, 3, MPI_INT, 0, endTaskTag, comm);
    }
}

// ============================================================
int main(int argc, char** argv) {
// ============================================================

    MPI_Init(&argc, &argv);
    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size < 2) {
        if (rank == 0)
            std::cerr << "Error: this test requires at least 2 MPI ranks.\n";
        MPI_Finalize();
        return 1;
    }

    const int numWorkers = size - 1;

    // ------------------------------------------------------------------
    // Command-line arguments
    // ------------------------------------------------------------------
    CmdLineArgParser cmdLine;
    cmdLine.set("-na",         5,         "Number of age groups");
    cmdLine.set("-nt",         5,         "Total number of time steps");
    cmdLine.set("-nm",         100,       "Mean sleep milliseconds per step");
    cmdLine.set("-ni", 10, "Sleep milliseconds when initialising a new cohort");
    cmdLine.set("-sd",         0.1,       "Sleep std-dev in ms (> 0)");
    cmdLine.set("-seed",       123456789, "Random seed");
    cmdLine.set("-nd",         10000,     "Number of doubles per chunk");
    cmdLine.set("-age_mature", 0,         "Index of first mature age class");

    bool parseOk = cmdLine.parse(argc, argv);
    bool help    = cmdLine.get<bool>("-help") || cmdLine.get<bool>("-h");
    if (!parseOk) {
        if (rank == 0) {
            std::cerr << "Error parsing command line arguments.\n";
            cmdLine.help();
        }
        MPI_Finalize();
        return 1;
    }
    if (help) {
        if (rank == 0) cmdLine.help();
        MPI_Finalize();
        return 0;
    }

    int    numAgeGroups = cmdLine.get<int>("-na");
    int    numTimeSteps = cmdLine.get<int>("-nt");
    int    milliseconds = cmdLine.get<int>("-nm");
    int init_milliseconds = cmdLine.get<int>("-ni");
    int    numData      = cmdLine.get<int>("-nd");
    int    seed         = cmdLine.get<int>("-seed") + rank;
    double sd           = cmdLine.get<double>("-sd");
    int    ageMature    = cmdLine.get<int>("-age_mature");

    std::mt19937 rng;
    rng.seed(seed);
    double k     = (double(milliseconds) * milliseconds) / (sd * sd);
    double theta = (sd * sd) / double(milliseconds);
    std::gamma_distribution<double> dist(k, theta);

    // ------------------------------------------------------------------
    // Cohort dependency graph — A+ ordering is handled by accumulate(),
    // no A+ entries needed in the graph.
    // ------------------------------------------------------------------
    SeapodymCohortDependencyAnalyzer taskDeps(
        numAgeGroups, numTimeSteps, ageMature, /*aPlusCohort=*/false);

    int numCohorts     = taskDeps.getNumberOfCohorts();
    int numCohortSteps = taskDeps.getNumberOfCohortSteps();
    std::map<int, int> stepBegMap    = taskDeps.getStepBegMap();
    std::map<int, int> stepEndMap    = taskDeps.getStepEndMap();
    std::map<int, std::set<std::array<int, 2>>> dependencyMap = taskDeps.getDependencyMap();

    if (rank == 0) {
        std::cout << "na=" << numAgeGroups << " nt=" << numTimeSteps
                  << " nd=" << numData << '\n';
        for (const auto& [task_id, beg] : stepBegMap) {
            std::cout << "  Task " << task_id
                      << "  steps " << beg << ".." << stepEndMap.at(task_id) - 1
                      << "  deps: ";
            for (const auto& [t2, s] : dependencyMap.at(task_id))
                std::cout << "(" << t2 << "," << s << ") ";
            std::cout << '\n';
        }
    }

    // ------------------------------------------------------------------
    // Data collectors — both on rank 0, single communicator.
    // ------------------------------------------------------------------
    int numChunks = numAgeGroups * numTimeSteps;
    DistDataCollector dataCollect(MPI_COMM_WORLD, numChunks, numData, /*rootRank=*/0);

    // A+ accumulator: single chunk, stored on rank 0.
    // Initialised to zero so MPI_Accumulate additions start from 0.
    DistDataCollector aplusCollect(MPI_COMM_WORLD, 1, numData, /*rootRank=*/0);
    if (rank == 0) {
        double* ptr = aplusCollect.getCollectedDataPtr();
        std::fill(ptr, ptr + numData, 0.0);
    }

    // ------------------------------------------------------------------
    // Bind task function and create worker.
    // ------------------------------------------------------------------
    auto taskFunc = std::bind(taskFunction,
        std::placeholders::_1,   // task_id
        std::placeholders::_2,   // stepBeg
        std::placeholders::_3,   // stepEnd
        std::placeholders::_4,   // comm
        milliseconds,
        init_milliseconds,
        numAgeGroups,
        numTimeSteps,
        numData,
        &dataCollect,
        &aplusCollect,
        &dependencyMap,
        &rng,
        &dist);

    TaskStepWorker worker(MPI_COMM_WORLD, taskFunc, stepBegMap, stepEndMap);

    MPI_Barrier(MPI_COMM_WORLD); // ensure A+ buffer is zeroed before any worker starts

    // ------------------------------------------------------------------
    // Manager / workers
    // ------------------------------------------------------------------
    if (rank == 0) {

        TaskStepManager manager(MPI_COMM_WORLD, numCohorts,
                                stepBegMap, stepEndMap, dependencyMap);

        double tic = MPI_Wtime();
        const auto results = manager.run();
        double toc = MPI_Wtime();

        auto numTotalSteps = results.size();
        double speedup = 0.001 * double(numTotalSteps * milliseconds) / (toc - tic);
        std::cout << "Execution time: " << toc - tic
                  << "  Speedup: "      << speedup
                  << "  Ideal: "        << numWorkers
                  << "  Parallel eff: " << speedup / double(numWorkers) << '\n';

        assert(numTotalSteps == (size_t)numCohortSteps);
        for (auto [taskId, step, res] : results)
            assert(taskId == res);

        std::cout << "Success\n";

    } else {
        worker.run();
    }

    // ------------------------------------------------------------------
    // Checksums — both on rank 0 (all data lives there).
    // ------------------------------------------------------------------
    if (rank == 0) {
        // Normal cohort checksum.
        // Expected for na=5, nt=10, nd=100000: 32500000
        {
            double* data    = dataCollect.getCollectedDataPtr();
            int     numSize = dataCollect.getNumSize();
            double  checksum = 0.0;
            for (int chunk = 0; chunk < dataCollect.getNumChunks(); ++chunk)
                checksum += std::accumulate(&data[chunk * numSize],
                                            &data[(chunk + 1) * numSize], 0.0);
            printf("\ndataCollect checksum: %.0lf\n", checksum);
        }

        // A+ accumulator checksum.
        // Final value = sum of feeder outputs = sum(0..nt-2) per element.
        // Expected for nt=5,  nd=10000:   60000   (0+1+2+3  = 6)
        // Expected for nt=10, nd=100000:  3600000 (0+1+..+8 = 36)
        {
            double* aplusData = aplusCollect.getCollectedDataPtr();
            int     numSize   = aplusCollect.getNumSize();
            double  checksum  = std::accumulate(aplusData, aplusData + numSize, 0.0);
            printf("aplusCollect accumulator (chunk 0) checksum: %.0lf\n", checksum);
        }
    }

    // ------------------------------------------------------------------
    // Cleanup
    // ------------------------------------------------------------------
    dataCollect.free();
    aplusCollect.free();

    MPI_Finalize();
    return 0;
}
