#pragma once

#include <vector>

// #include "core/engine.hpp"
#include "core/types.hpp"
#include "invariants/invariant.hpp"
// #include "variables/intVar.hpp"

class Engine;
class Invariant;

/**
 * Invariant for y <- array[index] where array is a vector of constants.
 *
 */

class ElementConst : public Invariant {
 private:
  VarId _index;
  std::vector<Int> _array;
  VarId _y;

 public:
  ElementConst(VarId index, std::vector<Int> array, VarId y);
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  VarId nextParameter(Timestamp, Engine&) override;
  void notifyCurrentParameterChanged(Timestamp, Engine&) override;
  void commit(Timestamp, Engine&) override;
};
