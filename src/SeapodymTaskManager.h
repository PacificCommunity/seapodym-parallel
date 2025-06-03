#include <map>
#include <vector>
#include <set>

#ifndef SEAPODYM_TASK_MANAGER
#define SEAPODYM_TASK_MANAGER

/**
 * Class SeapodymTaskManager
 * @brief The SeapodymTaskManager knows how to distribute tasks across workers, how many times a tasks needs to be executed 
 *        and what the next task should be given to a worker.
 */

class SeapodymTaskManager {

    private:

        // number of age groups for each time step, ideally matching the number of workers
        int numAgeGroups; 

        // number of workers 0...numWorkers-1
        int numWorkers;

        // total number of time steps across all cohorts
        int numTimeSteps;

        // number of cohorts
        int numCohorts = this->numAgeGroups + this->numTimeSteps - 1;

        // assign tasks to workers
        std::map<int, std::vector<int> > worker2task;

    public:

        /**
         * Constructor
         * @param numAgeGroups number of age groups that are run concurrently
         * @param numWorkers number of workers
         * @param numTimeSteps total number of time steps of the simulation
         * @return list of tasks
         */
        SeapodymTaskManager(int numAgeGroups, int numWorkers, int numTimeSteps);

        /**
         * Get the initial list of tasks
         * @param workerId worker ID
         * @return list of tasks
         */
        std::vector<int> getInitTaskIds(int workerId) const;

        /**
         * Get the number of current steps a task will run
         * @param taskId task ID
         * @return number of steps
         */
        int getNumSteps(int taskId) const;

        /**
         * Get the dependencies of a new task on othe preceeding tasks
         * @param taskId task ID
         * @return all the other tasks that feed into this task
         */
        std::set<int> getDependencies(int taskId) const;

        /**
         * Get the task that follows a terminated task
         * @param taskId task ID
         * @return the next task
         */
        int getNextTask(int taskId) const;

};

#endif // SEAPODYM_TASK_MANAGER
