#ifndef __STATISTICS_HPP_INCLUDED__
#define __STATISTICS_HPP_INCLUDED__
#include "model.hpp"

class Statistics {
 public:
  Statistics(Model* model);
  void countDefinedVariables(bool labels);
  void variablesDefinedBy();
  // void cyclesRemoved();
  void allStats(bool labels);
  void constraints(bool labels);

  Model* _model;
};

#endif
