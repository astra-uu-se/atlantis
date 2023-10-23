#pragma once

#include <memory>
#include <vector>

#include "propagation/solver.hpp"
#include "propagation/views/intView.hpp"

namespace atlantis::propagation {

class NotEqualConst : public IntView {
 private:
  const Int _val;

 public:
  explicit NotEqualConst(SolverBase& solver, VarId parentId, Int val)
      : IntView(solver, parentId), _val(val) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation