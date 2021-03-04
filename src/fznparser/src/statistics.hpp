#pragma once
#include "model.hpp"

class Statistics {
 public:
  Statistics(Model* model);
  void countDefinedVariables();
  void variablesDefinedBy();
  void cyclesRemoved();
  void allStats();
  void constraints();

  Model* _model;
};
