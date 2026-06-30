#include "SeapodymCohortDependencyAnalyzer.h"

SeapodymCohortDependencyAnalyzer::SeapodymCohortDependencyAnalyzer(int numAgeGroups, int numTimeSteps,
                                  int ageMature, bool aPlusCohort) {

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

        // Add the A+ cohorts. Each A+ cohort is treated exactly like any other
        // one-step cohort (same as the initial cohorts 0..numAgeGroups-1): it
        // gets a plain, positive Id, a single step (stepBeg=0, stepEnd=1), and
        // an entry in dependencyMap. A+ Ids are simply appended right after the
        // last normal Id, so there is no separate negative-Id namespace to
        // reason about downstream.
        int firstAPlusId = this->numIds; // == numAgeGroups + numTimeSteps - 1

        // A+ at time step 0 (Id = firstAPlusId) has no dependency; it is the
        // first thing that can run, just like the initial normal cohorts.
        this->dependencyMap[firstAPlusId] = std::set< std::array<int, 2>>();
        this->stepBegMap[firstAPlusId] = 0;
        this->stepEndMap[firstAPlusId] = 1;

        for (auto timeStep = 1; timeStep < this->numTimeSteps; ++timeStep) {

            int aPlusId = firstAPlusId + timeStep;

            // depends on the oldest age class at the previous time step and on the A+ cohort at the previous step
            this->dependencyMap[aPlusId] = std::set< std::array<int, 2>>{{timeStep - 1, this->numAgeGroups - 1}, {aPlusId - 1, 0}};

            // treat each A+ cohort at each step as a separate cohort with a single step
            this->stepBegMap[aPlusId] = 0;
            this->stepEndMap[aPlusId] = 1;
        }

        // update the number of Ids
        this->numIds += this->numTimeSteps;

        // add the dependency of new ("living") cohorts on the matching A+ cohort
        for (auto& [task_id, deps] : this->dependencyMap) {

            if (task_id < this->numAgeGroups) continue; // initial cohorts have no A+ dependency
            if (task_id >= firstAPlusId) continue;       // A+ cohorts themselves have no A+ dependency

            // e.g., if numAgeGroups == 5, (6,0) depends on (firstAPlusId+1, 0)
            deps.insert(std::array<int, 2>{firstAPlusId + (task_id - this->numAgeGroups), 0});
        }
    }

}

int 
SeapodymCohortDependencyAnalyzer::getNumberOfCohorts() const {
    return this->numIds;
}

int
SeapodymCohortDependencyAnalyzer::getNumberOfCohortSteps() const {
    int total = 0;
    for (const auto& [id, beg] : this->stepBegMap) {
        total += this->stepEndMap.at(id) - beg;
    }
    return total;
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

int
SeapodymCohortDependencyAnalyzer::getFirstAPlusCohortId() const {
    return this->numAgeGroups + this->numTimeSteps - 1;
}
