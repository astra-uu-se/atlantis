#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "types.hpp"
#include "propagation/solver.hpp"
#include "propagation/variables/committableInt.hpp"
#include "propagation/views/intView.hpp"

namespace atlantis::propagation {

/**
 * Invariant for y <- array[index] where array is a vector of constants.
 * NOTE: the index set is 1 based (first element is array[1], not array[0])
 *
 */

class ElementConst : public IntView {
 private:
  const std::vector<Int> _array;
  const Int _offset;

  [[nodiscard]] inline size_t safeIndex(Int index) noexcept {
    return std::max<Int>(Int(0),
                         std::min<Int>(static_cast<Int>(_array.size()) - Int(1),
                                       index - _offset));
  }

 public:
  explicit ElementConst(SolverBase& solver, VarId parentId, std::vector<Int> array,
                        Int offset = 1);
  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation