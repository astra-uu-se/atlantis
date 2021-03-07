#pragma once
#include "model.hpp"

class Statistics {
 public:
  Statistics(Model* model);
  void countDefinedVariables(bool labels);
  void variablesDefinedBy();
  void cyclesRemoved();
  void allStats(bool labels);
  void constraints(bool labels);

  Model* _model;
};
