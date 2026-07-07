/**
 * testTaskStepFarmingCohortAPlus3.cxx
 *
 * Like testTaskStepFarmingCohortAPlus2.cxx but the A+ cohort is NOT part of
 * the SeapodymCohortDependencyAnalyzer dependency graph (aPlusCohort=false).
 * Instead, the required ordering is enforced by a blocking ping-pong between
 * each feeder cohort and the A+ worker:
 *
 *   feeder at step na-1:
 *     1. MPI_Send NOTIFY(payload) → A+ worker   (the message itself carries
 *                                                the feeder's density data;
 *                                                wakes the A+ worker up)
 *     2. MPI_Recv ACK  ← A+ worker              (wait until accumulation done)
 *     3. MPI_Send END_TASK_TAG → manager        (only now does manager know
 *                                                feeder is finished)
 *
 * Because the manager learns the feeder is done only AFTER step 3, any new
 * cohort it assigns after that point is guaranteed to see an up-to-date
 * A+ accumulator — no A+ entry in the dependency graph is needed.
 *
 * There is no shared RMA input slot: earlier revisions of this test staged
 * the feeder's data in a single shared "chunk 0" that the A+ worker then
 * read, which left a theoretical race if two feeders' NOTIFYs happened to
 * overlap (a second put could clobber the first before it was consumed).
 * Embedding the payload directly in the NOTIFY message removes that window
 * entirely: each MPI_Recv on the A+ worker consumes exactly one sender's
 * complete message, so there is nothing for a second sender to overwrite.
 *
 * Communicator split:
 *   comm_farm  (ranks 0 .. size-2) — TaskStepManager / TaskStepWorker / dataCollect
 *   MPI_COMM_WORLD                 — NOTIFY(payload)/ACK signals with the A+ worker
 *
 * Requires >= 3 MPI ranks:
 *   rank 0          – manager
 *   ranks 1..size-2 – normal cohort workers
 *   rank size-1     – A+ worker (independent loop, not in dep graph)
 *
 * The A+ worker keeps its running accumulator in ordinary local memory (a
 * std::vector, not an RMA window) — no other rank ever reads it, so there is
 * nothing to publish over MPI between A+ steps.
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

// Tags for the A+ ping-pong protocol (must not clash with Tags.h: 0-3)
#define APLUS_NOTIFY_TAG 10
#define APLUS_ACK_TAG    11

/**
 * Chunk index for normal cohorts in dataCollect.
 */
int inline getChunkId(int task_id, int step, int na) {
    int row = task_id + step - na + 1;
    int col = task_id % na;
    return row * na + col;
}

/**
 * Task function for normal cohorts (task_id >= 0), excluding A+
 *
 * @param comm            farming communicator (comm_farm) – used for all
 *                        manager notifications (END_TASK_TAG etc.)
 * @param worldComm       MPI_COMM_WORLD – used for NOTIFY(payload)/ACK with
 *                        the A+ worker
 * @param aPlusWorkerRank rank of the A+ worker in MPI_COMM_WORLD (= size-1)
 */
void inline
taskFunction(int task_id, int stepBeg, int stepEnd, MPI_Comm comm,
    MPI_Comm worldComm, int aPlusWorkerRank,
    int init_milliseconds, int numAgeGroups, int numTimeSteps, int numData,
    DistDataCollector* dataCollector,
    std::map<int, std::set<std::array<int, 2>>>* dependencyMap,
    std::mt19937* rng, std::gamma_distribution<double>* dist) {

    const int endTaskTag = 1; // END_TASK_TAG

    std::vector<double> localData(numData, 0.0);
    std::vector<double> data(numData, 0.0);

    // ------------------------------------------------------------------
    // Gather initial conditions from explicit (normal-cohort) dependencies.
    // ------------------------------------------------------------------
    for (const auto& [task_id2, step] : (*dependencyMap)[task_id]) {
        int chunk_id = getChunkId(task_id2, step, numAgeGroups);
        dataCollector->get(chunk_id, data.data());

        if (!data.empty() && data.back() == dataCollector->BAD_VALUE) {
            MPI_Abort(worldComm, 1);
        }
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

        // Pretend we computed data; result is task_id for every element.
        std::fill(localData.begin(), localData.end(), double(task_id));

        // Publish result to the main data collector (on rank 0 of comm_farm).
        int chunk_id = getChunkId(task_id, step, numAgeGroups);
        dataCollector->put(chunk_id, localData.data());

        // ------------------------------------------------------------------
        // A+ ping-pong: feeder cohorts 0..numTimeSteps-2 at step na-1.
        // ------------------------------------------------------------------
        if (step == numAgeGroups - 1 && task_id <= numTimeSteps - 2) {

            // 1. Notify A+ worker, embedding this feeder's data directly in
            //    the message. No shared slot exists, so there is nothing for
            //    a concurrent feeder's NOTIFY to clobber.
            MPI_Send(localData.data(), numData, MPI_DOUBLE,
                     aPlusWorkerRank, APLUS_NOTIFY_TAG, worldComm);

            // 2. Block until A+ worker has finished accumulating.
            //    Only after this ACK does the manager learn this step is done.
            int dummy;
            MPI_Recv(&dummy, 1, MPI_INT,
                     aPlusWorkerRank, APLUS_ACK_TAG, worldComm, MPI_STATUS_IGNORE);
        }

        // 3. Notify the manager that this step is complete.
        //    For feeder steps, this happens AFTER the ACK, so the manager
        //    only unblocks downstream cohorts once the A+ worker is updated.
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

    const int aPlusWorkerRank = size - 1;
    const int farmWorkers     = size - 2; // workers in comm_farm, excl. manager

    // ------------------------------------------------------------------
    // Command-line arguments
    // ------------------------------------------------------------------
    CmdLineArgParser cmdLine;
    cmdLine.set("-na",        5,         "Number of age groups");
    cmdLine.set("-nt",        5,         "Total number of time steps");
    cmdLine.set("-nm",        100,       "Mean sleep milliseconds per step");
    cmdLine.set("-ni", 10, "Sleep milliseconds when initialising a new cohort");
    cmdLine.set("-sd",        0.1,       "Sleep standard deviation in ms (> 0)");
    cmdLine.set("-seed",      123456789, "Random seed");
    cmdLine.set("-nd",        10000,     "Number of doubles exchanged per chunk");
    cmdLine.set("-age_mature", 0,        "Index of the first mature age class");

    bool parseOk = cmdLine.parse(argc, argv);
    bool help    = cmdLine.get<bool>("-help") || cmdLine.get<bool>("-h");
    if (!parseOk) {
        std::cerr << "Error parsing command line arguments.\n";
        cmdLine.help();
        MPI_Finalize();
        return 1;
    }
    if (help && rank == 0) {
        cmdLine.help();
        MPI_Finalize();
        return 0;
    }

    // Need >= 3 ranks: manager + >= 1 normal worker + 1 A+ worker.
    if (size < 3) {
        if (rank == 0)
            std::cerr << "Error: this test requires at least 3 MPI ranks.\n";
        MPI_Finalize();
        return 1;
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
    // Cohort dependency graph — A+ is NOT included here.
    // The ordering A+ requires is provided by the ping-pong protocol.
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
                  << " nd=" << numData
                  << " A+ worker rank=" << aPlusWorkerRank << '\n';
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
    // Split communicator: ranks 0..size-2 form comm_farm.
    // Rank size-1 gets MPI_COMM_NULL.
    // ------------------------------------------------------------------
    MPI_Comm comm_farm;
    MPI_Comm_split(MPI_COMM_WORLD,
                   (rank < size - 1) ? 0 : MPI_UNDEFINED,
                   rank, &comm_farm);

    int numChunks = numAgeGroups * numTimeSteps;

    // ------------------------------------------------------------------
    // Branch: farming ranks vs A+ worker
    // ------------------------------------------------------------------
    if (rank < size - 1) {

        // --------------------------------------------------------------
        // Farming ranks (0 .. size-2)
        // --------------------------------------------------------------

        // Main data collector: normal cohort results on rank 0 of comm_farm.
        DistDataCollector dataCollect(comm_farm, numChunks, numData);

        auto taskFunc = std::bind(taskFunction,
            std::placeholders::_1,   // task_id
            std::placeholders::_2,   // stepBeg
            std::placeholders::_3,   // stepEnd
            std::placeholders::_4,   // comm  (= comm_farm, passed by worker)
            MPI_COMM_WORLD,          // worldComm
            aPlusWorkerRank,
            init_milliseconds,
            numAgeGroups,
            numTimeSteps,
            numData,
            &dataCollect,
            &dependencyMap,
            &rng,
            &dist);

        TaskStepWorker worker(comm_farm, taskFunc, stepBegMap, stepEndMap);
        MPI_Barrier(comm_farm); // sync farming ranks before starting

        if (rank == 0) {

            TaskStepManager manager(comm_farm, numCohorts,
                                    stepBegMap, stepEndMap, dependencyMap);

            double tic = MPI_Wtime();
            const auto results = manager.run();
            double toc = MPI_Wtime();

            auto numTotalSteps = results.size();
            double speedup = 0.001 * double(numTotalSteps * milliseconds) / (toc - tic);
            // Use size-1 (all non-manager ranks) as the ideal, consistent with
            // other APlus tests.  The A+ worker is a real MPI rank even though
            // it runs an independent loop rather than the task-step framework.
            const int allWorkers = size - 1;
            std::cout << "Execution time: " << toc - tic
                      << "  Speedup: "      << speedup
                      << "  Ideal: "        << allWorkers
                      << "  Parallel eff: " << speedup / double(allWorkers) << '\n';

            assert(numTotalSteps == (size_t)numCohortSteps);
            for (auto [taskId, step, res] : results)
                assert(taskId == res);

            std::cout << "Success\n";

        } else {
            worker.run();
        }

        // Checksum of the main data collector (rank 0 only).
        // Expected for na=5, nt=10, nd=100000: 32500000
        if (rank == 0) {
            double* data    = dataCollect.getCollectedDataPtr();
            int     numSize = dataCollect.getNumSize();
            double  checksum = 0.0;
            for (int chunk = 0; chunk < dataCollect.getNumChunks(); ++chunk)
                checksum += std::accumulate(&data[chunk * numSize],
                                            &data[(chunk + 1) * numSize], 0.0);
            printf("\ndataCollect checksum: %.0lf\n", checksum);
        }

        dataCollect.free(); // collective on comm_farm

    } else {

        // --------------------------------------------------------------
        // A+ worker (rank size-1): independent accumulation loop.
        //
        // Receives numTimeSteps-1 NOTIFY(payload) messages from feeder
        // cohorts (cohorts 0..numTimeSteps-2 at step na-1), accumulating
        // each one into a local running total. The accumulator lives in
        // ordinary process memory: no other rank ever reads it, so there
        // is no RMA window and nothing to publish between iterations.
        // --------------------------------------------------------------
        std::vector<double> accumulator(numData, 0.0);
        std::vector<double> incoming(numData);

        int dummy = 0;
        MPI_Status status;
        const int numAccumulations = numTimeSteps - 1; // feeder cohorts 0..nt-2

        for (int iter = 0; iter < numAccumulations; ++iter) {

            // Wait for a feeder cohort's NOTIFY; the message itself carries
            // that feeder's data, so there is no shared slot a second
            // feeder's message could clobber.
            MPI_Recv(incoming.data(), numData, MPI_DOUBLE,
                     MPI_ANY_SOURCE, APLUS_NOTIFY_TAG, MPI_COMM_WORLD, &status);

            // Accumulate (local memory, no RMA needed).
            for (int i = 0; i < numData; ++i)
                accumulator[i] += incoming[i];

            // Simulate A+ processing work using the same gamma distribution
            // as a normal step.  This must complete before the ACK so that
            // any dependent code path sees a fully processed result.
            int tsleep_aplus = static_cast<int>(std::round(dist(rng)));
            std::this_thread::sleep_for(std::chrono::milliseconds(tsleep_aplus));

            // Acknowledge: the feeder cohort will now notify the manager.
            MPI_Send(&dummy, 1, MPI_INT,
                     status.MPI_SOURCE, APLUS_ACK_TAG, MPI_COMM_WORLD);

        }

        // Checksum of the local accumulator, printed directly by this rank
        // (no RMA readback needed — this rank is the sole owner of the data).
        // Expected for nt=5,  nd=10000:   60000   (0+1+2+3     = 6)
        // Expected for nt=10, nd=100000:  3600000 (0+1+..+8    = 36)
        double checksum = std::accumulate(accumulator.begin(), accumulator.end(), 0.0);
        printf("[Rank %d] A+ accumulator checksum: %.0lf\n", rank, checksum);

    } // end rank split

    // ------------------------------------------------------------------
    // Synchronise all ranks before exiting.
    // ------------------------------------------------------------------
    MPI_Barrier(MPI_COMM_WORLD);

    // ------------------------------------------------------------------
    // Cleanup
    // ------------------------------------------------------------------
    if (comm_farm != MPI_COMM_NULL)
        MPI_Comm_free(&comm_farm);

    MPI_Finalize();
    return 0;
}
