#pragma once

#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {
class SolverBase;

class InIntervalConst : public IntView {
  Int _lb;
  Int _ub;

 public:
  explicit InIntervalConst(SolverBase& solver, VarViewId parentId, Int lb,
                           Int ub);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
