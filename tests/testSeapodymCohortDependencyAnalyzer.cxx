#include "SeapodymCohortDependencyAnalyzer.h"
#include "CmdLineArgParser.h"
#include <iostream>

int main(int argc, char** argv) {

    // Parse the command line arguments
    CmdLineArgParser cmdLine;
    cmdLine.set("-na", 3, "Number of age groups");
    cmdLine.set("-nt", 5, "Total number number of steps");
    bool success = cmdLine.parse(argc, argv);
    bool help = cmdLine.get<bool>("-help") || cmdLine.get<bool>("-h");
    if (!success) {
        std::cerr << "Error parsing command line arguments." << std::endl;
        cmdLine.help();
        return 1;
    }
    if (help) {
        cmdLine.help();
        return 1;
    }

    int numAgeGroups = cmdLine.get<int>("-na");
    int numTimeSteps = cmdLine.get<int>("-nt");

    SeapodymCohortDependencyAnalyzer depAnalyzer(numAgeGroups, numTimeSteps);
    int numCohorts = depAnalyzer.getNumberOfCohorts();
    std::map<int, std::set<std::array<int,2>>> dependencyMap = depAnalyzer.getDependencyMap();
    std::map<int, int> stepBegMap = depAnalyzer.getStepBegMap();
    std::map<int, int> stepEndMap = depAnalyzer.getStepEndMap();

    for (auto id = 0; id < numCohorts; ++id) {
        std::cout << "Cohort " << id << " has steps " << stepBegMap.at(id) << "... " << stepEndMap.at(id) - 1 << " and depends on: ";
        auto depSet = dependencyMap.at(id);
        for (const auto& [id2, step] : depSet) {
            std::cout << "(" << id2 << ", " << step << ") ";
        }
        std::cout << std::endl;
    }

    std::cout << "Success\n";
    return 0;
}
