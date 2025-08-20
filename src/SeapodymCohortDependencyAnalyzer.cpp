#include "SeapodymCohortDependencyAnalyzer.h"

SeapodymCohortDependencyAnalyzer::SeapodymCohortDependencyAnalyzer(int numAgeGroups, int numTimeSteps) {

    this->numAgeGroups = numAgeGroups;
    this->numTimeSteps = numTimeSteps;
    this->numTasks = numAgeGroups + numTimeSteps - 1;

    // set the number of steps for each task
    for (int task_id = 0; task_id < this->numTasks; ++task_id) {
        this->numStepsMap[task_id] = std::min(
            std::min(numAgeGroups, task_id + 1),
            numTimeSteps + numAgeGroups - task_id - 1
        );
    }

    // infer the dependency taskId => {[taskId, step], ...}

    // no dependency for task_id 0 ... numAgeGroups - 1 but still need to 
    // have a key for those tasks
    for (int task_id = 0; task_id < this->numAgeGroups; ++task_id) {
        std::set< std::array<int, 2>> dep_set;
        this->dependencyMap[task_id] = dep_set;
    }
    for (int task_id = this->numAgeGroups; task_id < this->numTasks; ++task_id) {

        std::set< std::array<int, 2>> dep_set;
        int step;

        for (int i = 0; i < this->numAgeGroups; ++i) {

            int otherTaskId = task_id - i - 1;

            if (otherTaskId < this->numAgeGroups - 1) {
                // special for the first row
                step = task_id - this->numAgeGroups;
            } else {
                // regular case
                step = i;
            }
            dep_set.insert(std::array<int, 2>{otherTaskId, step});
        }
    
        this->dependencyMap[task_id] = dep_set;
    }

}

int 
SeapodymCohortDependencyAnalyzer::getNumberOfCohorts() const {
    return this->numTasks;
}

std::map<int, int>
SeapodymCohortDependencyAnalyzer::getNumStepsMap() const {
    return this->numStepsMap;
}

std::map<int, std::set<std::array<int, 2>>> 
SeapodymCohortDependencyAnalyzer::getDependencyMap() const {
    return this->dependencyMap;
}