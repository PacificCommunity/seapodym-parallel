#include <SeapodymCohortFake.h>
#include <SeapodymCohortManager.h>
#include <SeapodymCourier.h>
#include <CmdLineArgParser.h>
#include <mpi.h>
#include <admodel.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <iostream>

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);
    int numWorkers;
    MPI_Comm_size(MPI_COMM_WORLD, &numWorkers);
    int workerId;
    MPI_Comm_rank(MPI_COMM_WORLD, &workerId);

    // times
    double tic, ttotStep = 0., ttotComm = 0.;
    
    CmdLineArgParser cmdLine;
    cmdLine.set("-na", 3, "Number of age groups");
    cmdLine.set("-nt", 5, "Total number of time steps");
    cmdLine.set("-nm", 15, "Milliseconds per step");
    cmdLine.set("-nd", 3, "Number of doubles in the data array");
    bool success = cmdLine.parse(argc, argv);
    bool help = cmdLine.get<bool>("-help") || cmdLine.get<bool>("-h");
    if (!success) {
        std::cerr << "Error parsing command line arguments." << std::endl;
        cmdLine.help();
        MPI_Finalize();
        return 1;
    }
    if (help) {
        cmdLine.help();
        MPI_Finalize();
        return 1;
    }

    // Number of age sworkerIdgroups, time steps, milliseconds per step, and number of doubles
    // These values are set by the command line arguments
    int numAgeGroups = cmdLine.get<int>("-na");
    int numTimeSteps = cmdLine.get<int>("-nt");
    int numMilliseconds = cmdLine.get<int>("-nm");
    int dataSize = cmdLine.get<int>("-nd");
    if (numAgeGroups <= 0 || numTimeSteps <= 0 || numMilliseconds <= 0 || dataSize <= 0) {
        if (workerId == 0) {
            // Print an error message only from the master worker
            std::cerr << "Invalid command line arguments. All values must be strictly positive integers." << std::endl;
        }
        cmdLine.help();
        MPI_Finalize();
        return 1;
    }
    if (numAgeGroups <= 0 || numTimeSteps <= 0 || numAgeGroups < numWorkers) {
        if (workerId == 0) {
            // Print an error message only from the master worker
            std::cerr << "Invalid number of age groups (" << 
            numAgeGroups << "), time steps (" << numTimeSteps << ") or workers (" << 
            numWorkers << ").\n";
            std::cerr << "Number of age groups must be > 0 and >= number of workers." << std::endl;
            cmdLine.help();
        }
        MPI_Finalize();
        return 1;
    }

    std::string sworkerId = std::to_string(workerId);
    // Use true to clobber the previously written log files
    auto logger = spdlog::basic_logger_mt(sworkerId, "logs/log_worker" + sworkerId + ".txt", true);
    logger->set_level(spdlog::level::debug);

    if (workerId == 0) {
        logger->info("Running with {} workers", numWorkers);
        logger->info("Number of age groups: {}, Total number of time steps: {}", numAgeGroups, numTimeSteps);
    }

    SeapodymCohortManager cohortManager(numAgeGroups, numWorkers, numTimeSteps);
    SeapodymCourier courier(MPI_COMM_WORLD);

    // Set the data value to the 100*workerId (for example)
    std::vector<double> data(dataSize, 100*workerId);
    courier.expose(data.data(), dataSize);

    std::vector<int> cohortIds = cohortManager.getInitCohortIds(workerId);
    std::vector<int> ageIndices = cohortIds; // Initialially, the age indices are the same as the cohort IDs

    // Initialize the step counter for each cohort task
    std::vector<int> step_counter(cohortIds.size(), 0);

    // Initialize the number of steps for each cohort task
    std::vector<int> cohortNumSteps(cohortIds.size());
    std::vector<SeapodymCohortFake*> cohortsPerWorker(cohortIds.size());
    for (auto i = 0; i < cohortIds.size(); ++i) {
        cohortNumSteps[i] = cohortManager.getNumSteps(cohortIds[i]);
        cohortsPerWorker[i] = new SeapodymCohortFake(numMilliseconds, dataSize, cohortIds[i]);
    }

    // Iterate over the global time steps
    for (auto istep = 0; istep < numTimeSteps; ++istep) {

        // Iterate over the cohort tasks assigned to this worker
        for (auto icohort = 0; icohort < cohortIds.size(); ++icohort) {
        
            // Get the cohort Id
            int cohortId = cohortIds[icohort];

            // Get the number of steps for this cohort task
            int numSteps = cohortNumSteps[icohort];

            int age = (istep + ageIndices[icohort]) % numAgeGroups;
            logger->info("starting processing cohort {} (age {}) at time step {}...", cohortId, age, istep);  
            
            // Simulate the time taken for a step
            tic = MPI_Wtime();
            cohortsPerWorker[icohort]->stepForward(dvar_vector());
            ttotStep += MPI_Wtime() - tic;

            // Update the data for this step
            data.assign(dataSize, 100*workerId + istep);

            logger->info("done processing cohort {} at time step {}", cohortId, istep);  

            // Done with the step
            step_counter[icohort]++;

            // Find out whether we have to switch to another cohort task
            if (step_counter[icohort] >= numSteps) {
                // Switch over to a new cohort task, reset the step counter and the number of steps
                cohortIds[icohort] = cohortManager.getNextCohort(cohortId);
                step_counter[icohort] = 0;
                cohortNumSteps[icohort] = cohortManager.getNumSteps(cohortIds[icohort]);
            }

        }

        // Accumulate the data from all workers
        int newCohortWorkerId = cohortManager.getNewCohortWorker(istep);
        logger->info("new cohort created by worker {} after time step {}", 
            newCohortWorkerId, istep);

#ifdef SEAPODYM_FETCH
        // Individual fetches
        MPI_Barrier(MPI_COMM_WORLD); // Ensure all workers are synchronized before fetching
        logger->info("starting fetching data from all workers at time step {}", istep);
        std::vector<double> sum_data(dataSize, 0);
        if (workerId == newCohortWorkerId) {
            for (auto iw = 0; iw < numWorkers; ++iw) {
                tic = MPI_Wtime();
                std::vector<double> fetchedData = courier.fetch(iw);
                logger->info("fetched data from worker {} at time step {}", iw, istep);
                for (int i = 0; i < dataSize; ++i) {
                    sum_data[i] += fetchedData[i]; // Sum the data from all workers
                }
                ttotComm += MPI_Wtime() - tic;
            }
            logger->info("done fetching data after time step {}", istep);
        }
#else
        // Accumulate
        logger->info("starting accumulation of data from all workers at time step {}", istep);

        tic = MPI_Wtime();
        std::vector<double> sum_data = courier.accumulate(newCohortWorkerId);
        ttotComm += MPI_Wtime() - tic;
        if (workerId == newCohortWorkerId) {
            logger->info("done accumulating data after time step {}", istep);
        }
#endif            

        if (workerId == newCohortWorkerId) {
            // Checksum
            double checksum = 0.0;
            for (int i = 0; i < dataSize; ++i) {
                checksum += sum_data[i];
            }
            logger->info("checksum from all workers at end of time step {}: {}", 
                istep, checksum);
            std::cout << "[" << workerId << "] @ step: " << istep << " checksum: " << checksum << '\n';
        }

    }

    // Total times for all the steps and the communication
    double ttotStepMin, ttotStepMax, ttotCommMin, ttotCommMax;
    MPI_Reduce(&ttotStep, &ttotStepMin, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&ttotStep, &ttotStepMax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&ttotComm, &ttotCommMin, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);                                                                                                                                    
    MPI_Reduce(&ttotComm, &ttotCommMax, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD); 
    if (workerId == 0) {
    std::cout << "Num workers: " << numWorkers << " num age groups: " << numAgeGroups << " num time steps: " << numTimeSteps << '\n';
    std::cout << "milliseconds per step: " << numMilliseconds << " data size: " << dataSize << '\n';
    std::cout << "Total times step: [" << ttotStepMin << ',' << ttotStepMax << 
        "] communication: [" << ttotCommMin << ',' << ttotCommMax << "]\n";
    }	    

    // clean up
    for (auto cohortPtr : cohortsPerWorker) {
        if (cohortPtr) {
            delete cohortPtr; // Free the memory allocated for the cohort
        }
    }
    courier.free(); // Free the MPI window

    MPI_Finalize();
    return 0;
}
