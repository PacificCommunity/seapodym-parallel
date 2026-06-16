#include "SeapodymCohortDependencyAnalyzer.h"

SeapodymCohortDependencyAnalyzer::SeapodymCohortDependencyAnalyzer(int numAgeGroups, int numTimeSteps, int ageMature, bool aPlusCohort) {

    this->numAgeGroups = numAgeGroups;
    this->numTimeSteps = numTimeSteps;
    this->numIds = numAgeGroups + numTimeSteps - 1;
    // a newborn cohort only reads mature ages of the previous time step, so younger
    // age classes (0..ageMature-1) are NOT real dependencies; including them delays
    // consecutive births and makes workers idle. This fix removes current cap ~na/2.
    if (ageMature < 0) ageMature = 0;
    if (ageMature >= numAgeGroups) ageMature = 0;   // guard against a degenerate value

    // set the min/max step indices
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

    // add "living" cohort dependencies
    for (int task_id = this->numAgeGroups; task_id < this->numIds; ++task_id) {

        std::set< std::array<int, 2>> dep_set;

        for (int step = ageMature; step < this->numAgeGroups; ++step) {

            int otherTaskId = task_id - step - 1;
            dep_set.insert(std::array<int, 2>{otherTaskId, step});

        }
    
        this->dependencyMap[task_id] = dep_set;
    }

    if (aPlusCohort) {
        // add the A+ cohort dependencies. To fit with the existing dependency representation, cohort Id: {(otherCohortId, step), ...}
        // the A+ cohort is treated as a separate cohort with a negative id at each time step. The (negative) id is -timeStep.
        for (auto timeStep = 1; timeStep < this->numTimeSteps; ++timeStep) {
            this->dependencyMap[-timeStep] = std::set< std::array<int, 2>>{{timeStep - 1, this->numAgeGroups - 1}};
        // Not sure if we need this
        this->stepBegMap[-timeStep] = 0;
        this->stepEndMap[-timeStep] = 1;
        }
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
