#pragma once

#include "atlantis/propagation/views/view.hpp"

namespace atlantis::propagation {

class SolverBase;  // Forward declaration

class IntView : public View {
 protected:
  friend class SolverBase;

 public:
  explicit IntView(SolverBase& solver, VarViewId parentId)
      : View(solver, parentId) {}

  void init(ViewId id) { _id = id; }

  [[nodiscard]] virtual Int value(Timestamp) = 0;
  [[nodiscard]] virtual Int committedValue() = 0;
  [[nodiscard]] virtual Int lowerBound() const = 0;
  [[nodiscard]] virtual Int upperBound() const = 0;
};

}  // namespace atlantis::propagation
