#pragma once

#include <memory>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {

class Violation2BoolView : public IntView {
 public:
  explicit Violation2BoolView(SolverBase& solver, const VarId parentId)
      : IntView(solver, parentId) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override { return 0; }
  [[nodiscard]] Int upperBound() const override { return 1; }
};

}  // namespace atlantis::propagation
