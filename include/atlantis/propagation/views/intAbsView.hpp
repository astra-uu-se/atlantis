#pragma once

#include <memory>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {

class IntAbsView : public IntView {
 public:
  IntAbsView(SolverBase& solver, const VarId parentId)
      : IntView(solver, parentId) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
