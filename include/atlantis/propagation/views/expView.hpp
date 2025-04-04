#pragma once

#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {
class SolverBase;

class ExpView : public IntView {
  Int _power;

 public:
  explicit ExpView(SolverBase& solver, VarViewId parentId, Int power)
      : IntView(solver, parentId), _power(power) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
