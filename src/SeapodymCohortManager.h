#include <map>
#include <vector>
#include <set>

#ifndef SEAPODYM_COHORT_MANAGER
#define SEAPODYM_COHORT_MANAGER

/**
 * Class SeapodymCohortManager
 * @brief The SeapodymTaskManager knows how to distribute task cohorts across workers. After each time step, 
 *        the workers are synchronized. This approach does not involve task farming -- the task are pre-assigned
 *        at the beginning of the simulation.
 */

class SeapodymCohortManager {

    private:

        // number of age groups for each time step, ideally matching the number of workers
        int numAgeGroups; 

        // number of workers 0...numWorkers-1
        int numWorkers;

        // total number of time steps across all cohorts
        int numTimeSteps;

        // number of cohorts
        int numCohorts = this->numAgeGroups + this->numTimeSteps - 1;

        // assign cohort Ids to workers
        std::map<int, std::vector<int> > worker2cohort;

        // map age index to worker ID
        std::map<int, int> ageIndex2Worker;

    public:

        /**
         * Constructor
         * @param numAgeGroups number of age groups that are run concurrently
         * @param numWorkers number of workers
         * @param numTimeSteps total number of time steps of the simulation
         * @return list of cohort tasks
         */
        SeapodymCohortManager(int numAgeGroups, int numWorkers, int numTimeSteps);

        /**
         * Get the initial list of cohort tasks
         * @param workerId worker ID
         * @return list
         */
        std::vector<int> getInitCohortIds(int workerId) const;

        /**
         * Get the number of current steps a cohort task will run
         * @param cohortId cohort task ID
         * @return number of steps
         */
        int getNumSteps(int cohortId) const;

        /**
         * Get the dependencies of a new cohort task on other preceding cohorts
         * @param cohortId cohort task ID
         * @return all the other cohort tasks that feed into this cohort task
         */
        std::set<int> getDependencies(int cohortId) const;

        /**
         * Get the cohort task that follows a terminated cohort task
         * @param cohortId cohort task ID
         * @return the next cohort task
         */
        int getNextCohort(int cohortId) const;

        /**
         * Get the worker ID for the new cohort
         * @param timeStep current time step
         * @return number
         */
        int getNewCohortWorker(int timeStep) const;

};

#endif // SEAPODYM_COHORT_MANAGER
