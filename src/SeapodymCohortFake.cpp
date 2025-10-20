#include "SeapodymCohortFake.h"

// to sleep...
#include <thread>
#include <chrono>

#define MILLISECONDS_STEP 15
#define DATA_SIZE 24000

SeapodymCohortFake::SeapodymCohortFake(const std::string& parFile, int id) {
  this->id = id;
  this->milliseconds_step = MILLISECONDS_STEP; // HARDWIRED
  this->data.resize(DATA_SIZE); // HARDWIRED
  // NEED TO PROCESS parFile
}

SeapodymCohortFake::SeapodymCohortFake(const std::string& restartFile) {
  this->milliseconds_step = MILLISECONDS_STEP; // HARDWIRED
  this->data.resize(DATA_SIZE); // HARDWIRED
  // NEED TO GET THE ID OUT OF THE RESTART FILE
}

SeapodymCohortFake::SeapodymCohortFake(const std::vector<double>& data, int id) {
  this->id = id;
  this->milliseconds_step = MILLISECONDS_STEP; // HARDWIRED
  this->setStateFromArray(data);
}

void 
SeapodymCohortFake::stepForward(const dvar_vector& paramVector) {
  // currently just sleep
  std::this_thread::sleep_for(std::chrono::milliseconds(this->milliseconds_step));
}

void SeapodymCohortFake::setStateFromArray(const std::vector<double>& array) {
  this->data = array; // copy
}

std::vector<double>
SeapodymCohortFake::getArrayFromState() const {
  // TO IMPLEMENT
  return std::vector<double>();
}

void 
SeapodymCohortFake::save(const std::string& restartFile) const {
  // TO IMPLEMENT
}
