#pragma once

#include <memory>
#include <vector>

#include "propagation/solver.hpp"
#include "propagation/views/intView.hpp"

namespace atlantis::propagation {

class IntMaxView : public IntView {
 private:
  Int _max;

 public:
  explicit IntMaxView(SolverBase& solver, VarId parentId, Int max)
      : IntView(solver, parentId), _max(max) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation