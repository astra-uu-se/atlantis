#pragma once

#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {
class SolverBase;

class Violation2BoolView : public IntView {
 public:
  explicit Violation2BoolView(SolverBase& solver, VarViewId parentId);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
