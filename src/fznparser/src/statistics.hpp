#pragma once
#include "model.hpp"

class Statistics {
 public:
  Statistics(Model* model);
  void countDefinedVariables();
  void checkCycles();

  Model* _model;
};
