#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- varArray[index] where varArray is a vector of VarId.
 * NOTE: the index set is 1 based (first element is varArray[1], not
 * varArray[0])
 *
 */

class Element2dVar : public Invariant {
 private:
  std::vector<std::vector<VarId>> _varMatrix;
  std::array<const VarId, 2> _indices;
  std::array<const Int, 2> _dimensions;
  std::array<const Int, 2> _offsets;
  VarId _output;

  [[nodiscard]] inline size_t safeIndex(Int index, size_t pos) const noexcept {
    return std::max<Int>(0,
                         std::min(_dimensions[pos] - 1, index - _offsets[pos]));
  }

  [[nodiscard]] inline size_t safeIndex1(Int index) const noexcept {
    return safeIndex(index, 0);
  }

  [[nodiscard]] inline size_t safeIndex2(Int index) const noexcept {
    return safeIndex(index, 1);
  }

 public:
  explicit Element2dVar(SolverBase&, VarId output, VarId index1, VarId index2,
                        std::vector<std::vector<VarId>>&& varMatrix,
                        Int offset1 = 1, Int offset2 = 1);
  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  [[nodiscard]] VarId dynamicInputVar(Timestamp) const noexcept override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  [[nodiscard]] VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
