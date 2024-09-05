#pragma once

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/intView.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

/**
 * In MiniZinc, the bool2int constraint enforces the following mapping:
 *  - 1 <-> true
 *  - 0 <-> false
 *
 *  However, to make it easier to use booleans in the violation of the search,
 *  we would like the mapping
 *  - 0 <-> true
 *  - 1 <-> false
 *
 *  This view can be used to ensure the MiniZinc mapping is used when the
 *  defined integer is the input for other constraints, where we want to use the
 *  MiniZinc mapping.
 */
class Bool2IntView : public IntView {
 public:
  explicit Bool2IntView(SolverBase& solver, const VarViewId parentId);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};

}  // namespace atlantis::propagation
