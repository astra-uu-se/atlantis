#pragma once

#include <vector>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;

/**
 * Invariant for b <- sum(A_i*X_i)
 *
 */

class Linear : public Invariant {
 private:
  std::vector<Int> _A;
  std::vector<VarId> _X;
  std::vector<SavedInt> _localX;
  VarId _b;

 public:
  Linear(std::vector<VarId> X, VarId b)
      : Linear(std::vector<Int>(X.size(), 1), X, b) {}
  Linear(std::vector<Int> A, std::vector<VarId> X, VarId b);

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  VarId getNextParameter(Timestamp, Engine&) override;
  void notifyCurrentParameterChanged(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
};
