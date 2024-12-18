#pragma once

#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {
class SolverBase;

class ScalarView : public IntView {
  Int _factor;
  Int _offset;

 public:
  explicit ScalarView(SolverBase& solver, VarViewId parentId, Int factor,
                      Int offset = 0);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
