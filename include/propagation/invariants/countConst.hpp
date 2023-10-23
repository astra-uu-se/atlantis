#pragma once

#include <utility>
#include <vector>

#include "types.hpp"
#include "propagation/engine.hpp"
#include "propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

/**
 * Invariant for:
 * output <- number of occurences of _y in _variables
 *
 */

class CountConst : public Invariant {
 private:
  const VarId _output;
  const Int _y;
  const std::vector<VarId> _variables;
  std::vector<Int> _hasCountValue;

 public:
  explicit CountConst(Engine&, VarId output, Int y,
                      std::vector<VarId> variables);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation