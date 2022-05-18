#pragma once

#include <vector>

#include "constraints/constraint.hpp"
#include "core/types.hpp"

class Engine;

/**
 * Invariant for y <- sum(coeffs_i*varArray_i)
 *
 */

class CountConst : public Constraint {
 private:
  const Int _y;
  const std::vector<VarId> _variables;
  std::vector<CommittableInt> _hasCountValue;

 public:
  explicit CountConst(VarId violationId, Int y, std::vector<VarId> variables);

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void close(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};
