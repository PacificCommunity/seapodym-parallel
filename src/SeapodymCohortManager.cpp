#include "SeapodymCohortManager.h"
#include <iostream>


SeapodymCohortManager::SeapodymCohortManager(int numAgeGroups, int numWorkers, int numTimeSteps) {

            this->numAgeGroups = numAgeGroups;
            this->numWorkers = numWorkers;
            this->numTimeSteps = numTimeSteps;

            for (auto wid = 0; wid < this->numWorkers; ++wid) {
                this->worker2cohort.insert({wid, std::vector<int>()});
            }

            // initially
            for (auto i = 0; i < this->numAgeGroups; ++i) {
                int workerId = i % this->numWorkers;
                this->worker2cohort[workerId].push_back(i);
            }

            for (auto ia = 0; ia < this->numAgeGroups; ++ia) {
                this->ageIndex2Worker[ia] = ia % this->numWorkers;
            }
        
        }

std::vector<int> 
SeapodymCohortManager::getInitCohortIds(int workerId) const {
    auto it = this->worker2cohort.find(workerId);
    return it->second;
}

int 
SeapodymCohortManager::getNumSteps(int cohortId) const {
            
    if (this->numAgeGroups <= cohortId && cohortId < this->numTimeSteps) {
        return this->numAgeGroups;
    }
    else if (cohortId >= this->numTimeSteps) {
        return this->numAgeGroups + this->numTimeSteps - cohortId - 1;
    }
    else {
        // if cohortId < this->numAgeGroups
        return this->numAgeGroups - cohortId;
    }
}

std::set<int> 
SeapodymCohortManager::getDependencies(int cohortId) const {

    std::set<int> res;
    
    if (cohortId > 2*this->numAgeGroups - 1) {
                
        for (auto i = cohortId - this->numAgeGroups + 1; i < cohortId; ++i) {
            res.insert(i);
        }

    }
    else if (this->numAgeGroups <= cohortId && cohortId <= 2*this->numAgeGroups - 1) {

        // first range: 0 to (2*na - 1 - cohort_id - 1)
        for (int i = 0; i < 2 * this->numAgeGroups - 1 - cohortId; ++i) {
            res.insert(i);
        }

        // second range: na to (cohort_id - 1)
        for (int i = this->numAgeGroups; i < cohortId; ++i) {
            res.insert(i);
        }
    }
        
    return res;
}

int 
SeapodymCohortManager::getNextCohort(int cohortId) const {
            
    int res = cohortId + this->numAgeGroups;
            
    if (cohortId < this->numAgeGroups) {
        res = cohortId + 2*(this->numAgeGroups - 1 - cohortId) + 1;
    }
    else if (cohortId >= this->numTimeSteps - 1) {
        //last cohort
        res = -1;
    }
    return res;
}

int
SeapodymCohortManager::getNewCohortWorker(int timeStep) const {
    // positive version of  (this->numAgeGroups - timeStep - 1) % this->numAgeGroups
    // unlike the % operator in Python, C++'s % operator can return negative values
    // so we need to ensure that the result is always positive
    int ageIndex = ( (this->numAgeGroups - timeStep - 1) % this->numAgeGroups + this->numAgeGroups ) % this->numAgeGroups;
    int workerId = this->ageIndex2Worker.at(ageIndex);
    return workerId;
}
