#pragma once

#include <vector>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;

/**
 * Invariant for y <- sum(coeffs_i*varArray_i)
 *
 */

class Linear : public Invariant {
 private:
  const std::vector<Int> _coeffs;
  const std::vector<VarId> _varArray;
  std::vector<CommittableInt> _localVarArray;
  const VarId _y;

 public:
  Linear(const std::vector<VarId>& varArray, VarId y)
      : Linear(std::vector<Int>(varArray.size(), 1), varArray, y) {}
  Linear(std::vector<Int> coeffs, std::vector<VarId> varArray, VarId y);

  void registerVars(Engine&) override;
  void updateBounds(Engine&) override;
  void close(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};
