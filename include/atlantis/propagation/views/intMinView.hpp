#pragma once

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/views/intView.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class IntMinView : public IntView {
 private:
  Int _min;

 public:
  explicit IntMinView(SolverBase& solver, VarViewId parentId, Int min);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
