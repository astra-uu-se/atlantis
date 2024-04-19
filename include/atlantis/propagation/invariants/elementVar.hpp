#pragma once

#include <vector>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- varArray[index] where varArray is a vector of
 * VarViewId. NOTE: the index set is 1 based (first element is varArray[1], not
 * varArray[0])
 *
 */

class ElementVar : public Invariant {
 private:
  VarId _output, _index;
  std::vector<VarViewId> _varArray;
  Int _offset;
  size_t _activeIndex;

  [[nodiscard]] inline size_t safeIndex(Int index) const noexcept {
    return std::max<Int>(
        0, std::min(static_cast<Int>(_varArray.size()) - 1, index - _offset));
  }

 public:
  explicit ElementVar(SolverBase&, VarId output, VarViewId index,
                      std::vector<VarViewId>&& varArray, Int offset = 1);

  explicit ElementVar(SolverBase&, VarViewId output, VarViewId index,
                      std::vector<VarViewId>&& varArray, Int offset = 1);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  [[nodiscard]] VarViewId dynamicInputVar(Timestamp) const noexcept override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
  void commit(Timestamp) override;
};

}  // namespace atlantis::propagation
