#pragma once

#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/variables/var.hpp"

namespace atlantis::propagation {

class SolverBase;

class View : public Var {
 protected:
  SolverBase& _solver;
  VarViewId _parentId;

 public:
  explicit View(SolverBase& solver, VarViewId parentId)
      : Var(NULL_ID), _solver(solver), _parentId(parentId) {}

  void setId(ViewId id) { _id = id; }

  [[nodiscard]] ViewId id() const { return _id; }

  [[nodiscard]] VarViewId parentId() const { return _parentId; }
};

}  // namespace atlantis::propagation
