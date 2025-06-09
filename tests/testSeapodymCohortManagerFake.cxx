#include <iostream>
#include <SeapodymCohortFake.h>
#include <SeapodymCohortManager.h>

/**
 * Test function
 * @param na Number of age groups (for each time step)
 * @param nw Number of workers
 * @param nt Total number of time steps
 */
void test(int na, int nw, int nt) {

    const int milliseconds_step = 15;
    const int data_size = 100;

    SeapodymCohortManager tm(na, nw, nt);
    dvar_vector no_param;

    // iterate over the workers
    for (auto iw = 0; iw < nw; ++iw) {
        std::cout << "test(" << na << ", " << nw << ", " << nt << ") worker " << iw << ": ";

        // get the initial tasks for this worker
        auto tasks = tm.getInitCohortIds(iw);

        // itereate over the inital tasks
        for (auto t : tasks) {

            // set the task
            int tid = t;
            SeapodymCohortFake* sc = new SeapodymCohortFake(milliseconds_step, data_size, tid);

            // step forward until the next task is < 0
            while (tid >= 0) {

                // get the number of steps for this task
                auto nsteps = tm.getNumSteps(tid);

                // march forward nsteps
                for (auto i = 0; i < nsteps; ++i) {
                    sc->stepForward(no_param);
                    std::cout << tid << " ";
                }

                // are there more tasks to execute?
                tid = tm.getNextCohort(tid);
                delete sc;
                sc = new SeapodymCohortFake(milliseconds_step, data_size, tid);
                auto deps = tm.getDependencies(tid);
                std::cout << "[";
                for (auto d : deps){
                    std::cout << d << " ";
                }
                std::cout << "] -> ";
            }
            std::cout << ", ";

            delete sc;
        }
        std:: cout << '\n';
    }
    std::cout << "=====================\n";
}

int main() {

    // 3 age groups, 3 workers, 5 steps
    test(3, 3, 5);

    // 3 age groups, 3 workers, 10 steps
    test(3, 3, 10);

    // 3 age groups, 2 workers, 5 steps
    test(3, 2, 5);

    return 0;
}
