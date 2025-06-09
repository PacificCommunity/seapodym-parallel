#include <iostream>
#include <SeapodymCohortManager.h>

void test(int na, int nw, int nt) {

    SeapodymCohortManager tm(na, nw, nt);

    // iterate over the workers
    for (auto iw = 0; iw < nw; ++iw) {
        std::cout << "test(" << na << ", " << nw << ", " << nt << ") worker " << iw << ": ";

        // get the initial tasks for this worker
        auto tasks = tm.getInitCohortIds(iw);

        // itereate over the inital tasks
        for (auto t : tasks) {

            // set the task
            int tid = t;

            // step forward until the next task is < 0
            while (tid >= 0) {

                // get the number of steps for this task
                auto nsteps = tm.getNumSteps(tid);

                // march forward nsteps
                for (auto i = 0; i < nsteps; ++i) {
                    std::cout << tid << " ";
                }

                // are there more tasks to execute?
                tid = tm.getNextCohort(tid);
                auto deps = tm.getDependencies(tid);
                std::cout << "[";
                for (auto d : deps){
                    std::cout << d << " ";
                }
                std::cout << "] -> ";
            }
            std::cout << ", ";
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
