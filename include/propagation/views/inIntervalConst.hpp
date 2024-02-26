#pragma once

#include <memory>
#include <vector>

#include "propagation/solver.hpp"
#include "propagation/views/intView.hpp"

namespace atlantis::propagation {

class InIntervalConst : public IntView {
 private:
  Int _lb;
  Int _ub;

 public:
  explicit InIntervalConst(SolverBase& solver, VarId parentId, Int lb, Int ub)
      : IntView(solver, parentId), _lb(lb), _ub(ub) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation