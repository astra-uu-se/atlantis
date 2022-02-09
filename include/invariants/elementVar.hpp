#pragma once

#include <vector>

#include "core/engine.hpp"
#include "core/types.hpp"
#include "invariants/invariant.hpp"

/**
 * Invariant for y <- varArray[index] where varArray is a vector of VarId.
 *
 */

class ElementVar : public Invariant {
 private:
  const VarId _index;
  const std::vector<VarId> _varArray;
  const VarId _y;

 public:
  ElementVar(VarId index, std::vector<VarId> varArray, VarId y);
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  VarId getNextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
  void commit(Timestamp, Engine&) override;
};
