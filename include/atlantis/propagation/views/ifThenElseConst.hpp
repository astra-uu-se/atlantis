#pragma once

#include <array>

#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {
class SolverBase;

class IfThenElseConst : public IntView {
  std::array<Int, 2> _values;
  Int _condVal;

 public:
  explicit IfThenElseConst(SolverBase& solver, VarViewId parentId, Int thenVal,
                           Int elseVal, Int condVal = 0);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
