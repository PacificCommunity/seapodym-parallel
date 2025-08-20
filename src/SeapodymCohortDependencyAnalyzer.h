#include <map>
#include <set>
#include <array>

#ifndef SEAPODYM_COHORT_DEPENDENCY_ANALYZER
#define SEAPODYM_COHORT_DEPENDENCY_ANALYZER

/**
 * @brief A class to compute the task dependencies of multiple fish cohort tasks
 * 
 * @details A fish cohort is a population of fish that were born in the same interval of time. 
 *          A task evolves a fish cohort over multiple time steps. After some time steps, the 
 *          cohort is removed from the population and a new cohort is being born by combining 
 *          the biomass of the cohorts from the previous time step. For instance, let there be 
 *          3 starting age groups (0, 1 and 2) whose populations are modelled over 5 time 
 *          steps. This can be represented as:
\verbatim
2 1 0
2 1 3
2 4 3
5 4 3
5 4 6
\endverbatim
 *          where the numbers are the cohort Ids. The horizontal axis represents the different
 *          age groups and the vertical axis time. A each time step a new cohort is born. 
 */
class SeapodymCohortDependencyAnalyzer {

private:

    // number of age groups
    int numAgeGroups;

    // number of time steps
    int numTimeSteps;

    // total number of cohorts
    int numIds;

    // cohort Id: number of steps
    std::map<int, int> numStepsMap;

    // cohort Id: {(taskId, step), ...}
    std::map<int, std::set<std::array<int, 2>>> dependencyMap;

public:

    /**
     * Constructor
     * 
     * @param numAgeGroups number of age groups (e.g. 3 in the above example)
     * @param numTimeSteps number of tiem steps (e.g. 5 in the above example)
     */
    SeapodymCohortDependencyAnalyzer(int numAgeGroups, int numTimeSteps);

    /**
     * Get the number of cohorts
     * 
     * In the above example, there are 7 cohorts with Ids 0..6
     * @return number
     */
    int getNumberOfCohorts() const;

    /**
     * Get the number of cohort steps
     * 
     * In the above example, there are 15 cohort-steps
     * @return number
     */
    int getNumberOfCohortSteps() const;

    /**
     * Get the cohort Id to number of steps map
     * 
     * In the above example, cohort 0 has 3 steps, cohort 1 has 2 steps, 
     * cohort 2 has 1 step, etc.
     * @return Id to number map
     */
    std::map<int, int> getNumStepsMap() const;

    /**
     * Get the dependency map
     * 
     * In the above example, cohorts 0, 1 and 2 have no dependencies. 
     * Cohort 3 depends on cohorts 0, 1, 2 all at step 0; cohort 4 depends
     * on cohort 0 at step 1, cohort 1 at step 1 and cohort 3 at step 0; etc.
     * @return Id: {(Id, step), ...} map
     */
    std::map<int, std::set<std::array<int, 2>>>  getDependencyMap() const;

};

#endif // SEAPODYM_COHORT_DEPENDENCY_ANALYZER