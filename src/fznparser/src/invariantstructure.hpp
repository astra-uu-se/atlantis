#include "model.hpp"
#include "schemes.hpp"
#include "statistics.hpp"

class InvariantStructure {
 public:
  InvariantStructure(Model m, std::string args);
  void run();
  void line() { _stats.line(); }

 private:
  Model _model;
  Statistics _stats;
  Schemes _schemes;
  bool _allStats;
  bool _fullStats;
  bool _ignoreDynamicCycles;
};
