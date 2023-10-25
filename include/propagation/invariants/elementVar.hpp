#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "types.hpp"
#include "propagation/solver.hpp"
#include "propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- varArray[index] where varArray is a vector of VarId.
 * NOTE: the index set is 1 based (first element is varArray[1], not
 * varArray[0])
 *
 */

class ElementVar : public Invariant {
 private:
  const VarId _output, _index;
  const std::vector<VarId> _varArray;
  const Int _offset;

  [[nodiscard]] inline size_t safeIndex(Int index) const noexcept {
    return std::max<Int>(
        0, std::min(static_cast<Int>(_varArray.size()) - 1, index - _offset));
  }

 public:
  explicit ElementVar(SolverBase&, VarId output, VarId index,
                      std::vector<VarId> varArray, Int offset = 1);
  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  VarId dynamicInputVar(Timestamp) const noexcept override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation