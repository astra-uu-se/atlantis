#pragma once

#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {
class SolverBase;

class IntAbsView : public IntView {
 public:
  IntAbsView(SolverBase& solver, VarViewId parentId);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
