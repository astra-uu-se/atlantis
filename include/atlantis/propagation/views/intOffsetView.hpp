#pragma once

#include <memory>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {

class IntOffsetView : public IntView {
 private:
  const Int _offset;

 public:
  explicit IntOffsetView(SolverBase& solver, VarId parentId, Int offset)
      : IntView(solver, parentId), _offset(offset) {}

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
