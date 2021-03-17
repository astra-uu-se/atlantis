#ifndef __STATISTICS_HPP_INCLUDED__
#define __STATISTICS_HPP_INCLUDED__
#include "model.hpp"

class Statistics {
 public:
  Statistics(Model* model);
  void countDefinedVariables(bool labels);
  void variablesDefinedBy();
  void allStats(bool labels);
  void constraints(bool labels);
  int variableCount();
  int definedCount();
  void width();
  void width_aux(Node* node, int x, int& w);

  Model* _model;
};

#endif
