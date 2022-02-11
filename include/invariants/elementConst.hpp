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
  const VarId _index;
  const std::vector<Int> _array;
  const VarId _y;

 public:
  ElementConst(VarId index, std::vector<Int> array, VarId y);

#ifndef CBLS_TEST
  void init(Timestamp, Engine&) final;
  void recompute(Timestamp, Engine&) final;
  void notifyIntChanged(Timestamp, Engine&, LocalId) final;
  void commit(Timestamp, Engine&) final;
  VarId getNextInput(Timestamp, Engine&) final;
  void notifyCurrentInputChanged(Timestamp, Engine&) final;
#else
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
#endif
};
