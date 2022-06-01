#pragma once

#include <vector>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;

/**
 * Invariant for:
 * output <- number of occurences of _y in _variables
 *
 */

class CountConst : public Invariant {
 private:
  const Int _y;
  const std::vector<VarId> _variables;
  const VarId _output;
  std::vector<CommittableInt> _hasCountValue;

 public:
  explicit CountConst(Int y, std::vector<VarId> variables, VarId output);

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void close(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};
