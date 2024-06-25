#pragma once

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/views/intView.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class IfThenElseConst : public IntView {
 private:
  std::array<Int, 2> _values;

 public:
  explicit IfThenElseConst(SolverBase& solver, VarId parentId, Int thenVal,
                           Int elseVal);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
