#pragma once

#include <vector>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;

/**
 * Invariant for output <- sum(coeffs_i * varArray_i)
 *
 */

class Linear : public Invariant {
 private:
  const VarId _output;
  const std::vector<Int> _coeffs;
  const std::vector<VarId> _varArray;
  std::vector<CommittableInt> _localVarArray;

 public:
  explicit Linear(VarId output, const std::vector<VarId>& varArray);
  explicit Linear(VarId output, std::vector<Int> coeffs,
                  std::vector<VarId> varArray);

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void close(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};
