#pragma once

#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {
class SolverBase;

class NotEqualConst : public IntView {
  Int _val;

 public:
  explicit NotEqualConst(SolverBase& solver, VarViewId parentId, Int val)
      : IntView(solver, parentId), _val(val) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
