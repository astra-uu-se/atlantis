#pragma once

#include <vector>

#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {
class SolverBase;

/**
 * Invariant for y <- array[index] where array is a vector of constants.
 * NOTE: the index set is 1 based (first element is array[1], not array[0])
 *
 */

class ElementConst : public IntView {
  std::vector<Int> _array;
  Int _offset;

  [[nodiscard]] size_t safeIndex(Int index) const noexcept {
    return std::max<Int>(Int(0),
                         std::min<Int>(static_cast<Int>(_array.size()) - Int(1),
                                       index - _offset));
  }

 public:
  explicit ElementConst(SolverBase& solver, VarViewId parentId,
                        std::vector<Int>&& array, Int offset = 1);
  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
