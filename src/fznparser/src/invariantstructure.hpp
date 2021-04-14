#pragma once

#include "model.hpp"
#include "schemes.hpp"
#include "statistics.hpp"

class InvariantStructure {
 public:
  InvariantStructure(Model m, std::string args);
  void run();
  void runSmall();
  void line() { _stats.line(); }

 private:
  Model _model;
  Statistics _stats;
  Schemes _schemes;
  bool _allStats;
  bool _fullStats;
  bool _ignoreDynamicCycles;
  bool _info;
  bool _small;
  bool _noStats;
};
