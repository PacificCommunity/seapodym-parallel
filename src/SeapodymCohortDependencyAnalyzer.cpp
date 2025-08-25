#include "SeapodymCohortDependencyAnalyzer.h"

SeapodymCohortDependencyAnalyzer::SeapodymCohortDependencyAnalyzer(int numAgeGroups, int numTimeSteps) {

    this->numAgeGroups = numAgeGroups;
    this->numTimeSteps = numTimeSteps;
    this->numIds = numAgeGroups + numTimeSteps - 1;

    // set the min/mas step indices
    for (int task_id = 0; task_id < this->numIds; ++task_id) {

        this->stepBegMap[task_id] = std::max(0, this->numAgeGroups - 1 - task_id);

        this->stepEndMap[task_id] = std::min(this->numAgeGroups, this->numTimeSteps + this->numAgeGroups - 1 - task_id);

    }

    // infer the dependency taskId => {[taskId, step], ...}

    // no dependency for task_id 0 ... numAgeGroups - 1 but still need to 
    // have a key for those tasks
    for (int task_id = 0; task_id < this->numAgeGroups; ++task_id) {
        // empty
        this->dependencyMap[task_id] = std::set< std::array<int, 2>>();
    }

    for (int task_id = this->numAgeGroups; task_id < this->numIds; ++task_id) {

        std::set< std::array<int, 2>> dep_set;

        for (int step = 0; step < this->numAgeGroups; ++step) {

            int otherTaskId = task_id - step - 1;
            dep_set.insert(std::array<int, 2>{otherTaskId, step});

        }
    
        this->dependencyMap[task_id] = dep_set;
    }

}

int 
SeapodymCohortDependencyAnalyzer::getNumberOfCohorts() const {
    return this->numIds;
}

int 
SeapodymCohortDependencyAnalyzer::getNumberOfCohortSteps() const {
    return this->numAgeGroups * this->numTimeSteps;
}

std::map<int, int>
SeapodymCohortDependencyAnalyzer::getStepBegMap() const {
    return this->stepBegMap;
}

std::map<int, int>
SeapodymCohortDependencyAnalyzer::getStepEndMap() const {
    return this->stepEndMap;
}

std::map<int, std::set<std::array<int, 2>>> 
SeapodymCohortDependencyAnalyzer::getDependencyMap() const {
    return this->dependencyMap;
}