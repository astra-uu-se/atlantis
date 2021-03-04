#pragma once
#include "model.hpp"

class Statistics {
 public:
  Statistics(Model* model);
  void countDefinedVariables();
  void variablesDefinedBy();
  void cyclesRemoved();

  Model* _model;
};
