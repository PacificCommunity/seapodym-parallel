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
(0,2) (1,1) (2,0) (-1,0)
(3,0) (1,2) (2,1) (-2,0)
(3,1) (4,0) (2,2) (-3,0)
(3,2) (4,1) (5,0) (-4,0)
(6,0) (4,2) (5,1) (-5,0)
\endverbatim
 *          where (i=cohort, j=step). The horizontal axis represents the different
 *          age groups (in this case 3) and the vertical axis is time. A each time step a new cohort is born. 
 *          A+ cohorts have i < 0 and, in this representation, run for only one step.
 * 
 *          Note that i + j = const across a row (normal cohorts only; A+ cohorts always have j=0).
 *
 *          Initially, (0,2) (1,1) (2,0) (-1,0) have no dependency and can be started immediately.
 *          Any new, "living" cohort depends on cohorts which have attained a maturity age "am"
 *          at the previous time step (in addition to the A+ cohort, if present). If am == 1 then
 *          (3,0) depends on (0,2) (1,1) (-1,0)
 *          (4,0) depends on (1,2) (2,1) (-2,0)
 *          (5,0) depends on (3,1) (2,2) (-3,0)
 *
 *          Therefore (i, 0) depends on (i-j-1, j) for j = am...na-1, and on (-(i-na)-1, 0)
 */
class SeapodymCohortDependencyAnalyzer {

private:

    // number of age groups
    int numAgeGroups;

    // number of time steps
    int numTimeSteps;

    // total number of cohorts (including A+ if aPlusCohort=true)
    int numIds;

    // cohort Id: starting step index
    std::map<int, int> stepBegMap;

    // cohort Id: ending step index + 1
    std::map<int, int> stepEndMap;

    // cohort Id: {(taskId, step), ...}
    std::map<int, std::set<std::array<int, 2>>> dependencyMap;

public:

    /**
     * Constructor
     * 
     * @param numAgeGroups number of age groups (e.g. 3 in the above example)
     * @param numTimeSteps number of tiem steps (e.g. 5 in the above example)
     * @param ageMature index of the first mature age class. A newborn cohort only
     *        needs the densities of mature age classes (ageMature..numAgeGroups-1)
     *        of the previous time step, so dependencies on younger ages are omitted.
     *        Defaults to 0 = dependence on the full state at time t-1.
     * @param aPlusCohort whether to add the A+ cohort dependency
     */
    SeapodymCohortDependencyAnalyzer(int numAgeGroups, int numTimeSteps, int ageMature = 0, bool aPlusCohort = false);

    /**
     * Get the total number of cohorts (normal + A+ if aPlusCohort=true)
     *
     * In the above example without A+: 7 cohorts (Ids 0..6).
     * With A+: 7 + 5 = 12 cohorts (Ids -5..-1 and 0..6).
     * @return number
     */
    int getNumberOfCohorts() const;

    /**
     * Get the total number of cohort steps across all cohorts
     *
     * In the above example without A+: 5*3 = 15 steps.
     * With A+: 5*3 + 5 = 20 steps.
     * @return number
     */
    int getNumberOfCohortSteps() const;

    /**
     * Get the cohort Id to first step index map
     *
     * In the above example (normal cohorts): 0->2, 1->1, 2->0, 3->0, 4->0, 5->0, 6->0.
     * A+ cohorts (if present) all map to 0: -1->0, -2->0, ..., -nt->0.
     * @return Id to number map
     */
    std::map<int, int> getStepBegMap() const;

    /**
     * Get the cohort Id to last step + 1 map
     *
     * In the above example (normal cohorts): 0->3, 1->3, 2->3, 3->3, 4->3, 5->2, 6->1.
     * A+ cohorts (if present) all map to 1: -1->1, -2->1, ..., -nt->1.
     * @return Id to number map
     */
    std::map<int, int> getStepEndMap() const;

    /**
     * Get the dependency map
     * 
     * @return Id: {(Id, step), ...} map
     */
    std::map<int, std::set<std::array<int, 2>>>  getDependencyMap() const;

};

#endif // SEAPODYM_COHORT_DEPENDENCY_ANALYZER
