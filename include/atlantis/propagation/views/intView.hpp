#pragma once

#include "atlantis/propagation/views/view.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class SolverBase;  // Forward declaration

class IntView : public View {
 protected:
  friend class SolverBase;
  // A raw pointer might be the best option here as views lifetime depend
  // on solver and not vice-versa:

 public:
  explicit IntView(SolverBase& solver, VarId parentId)
      : View(solver, parentId) {}

  void init(VarId id) { _id = id; }

  [[nodiscard]] virtual Int value(Timestamp) = 0;
  [[nodiscard]] virtual Int committedValue() = 0;
  [[nodiscard]] virtual Int lowerBound() const = 0;
  [[nodiscard]] virtual Int upperBound() const = 0;
};

}  // namespace atlantis::propagation
