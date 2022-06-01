#pragma once

#include <vector>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;

/**
 * Invariant for output <- sum(coeffs_i * violArray_i)
 *
 */

class BoolLinear : public Invariant {
 private:
  const VarId _output;
  const std::vector<Int> _coeffs;
  const std::vector<VarId> _violArray;
  // (_isSatisfied[i] == 1) <=> (_violArray[i] == 0)
  std::vector<CommittableInt> _isSatisfied;

 public:
  explicit BoolLinear(VarId output, const std::vector<VarId>& violArray);
  explicit BoolLinear(VarId output, std::vector<Int> coeffs,
                      std::vector<VarId> violArray);

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void close(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};
