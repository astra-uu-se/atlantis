#pragma once

#include <vector>

#include "core/engine.hpp"
#include "core/types.hpp"
#include "invariants/invariant.hpp"

/**
 * Invariant for b <- X[i] where X is a vector of VarId.
 *
 */

class ElementVar : public Invariant {
 private:
  VarId _i;
  std::vector<VarId> _X;
  VarId _b;

 public:
  ElementVar(VarId i, std::vector<VarId> X, VarId b);
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  VarId getNextDependency(Timestamp, Engine&) override;
  void notifyCurrentDependencyChanged(Timestamp, Engine&) override;
  void commit(Timestamp, Engine&) override;
};
