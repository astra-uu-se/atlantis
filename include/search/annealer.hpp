#pragma once

#include "core/types.hpp"
namespace search {

class Annealer {
 private:
  double _temperature;
  Int _localIterations;

 public:
  Annealer() : _temperature(1.0), _localIterations(1000) {}
  
  bool globalCondition();
  bool searchLocally();

  [[nodiscard]] bool accept(Int currentObjectiveValue,
                            Int newObjectiveValue) const;
};

}  // namespace search