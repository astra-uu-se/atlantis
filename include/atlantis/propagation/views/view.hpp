#pragma once

#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/variables/var.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class SolverBase;

class View : public Var {
 protected:
  SolverBase& _solver;
  VarViewId _parentId;

 public:
  explicit View(SolverBase& solver, VarViewId parentId)
      : Var(NULL_ID), _solver(solver), _parentId(parentId) {}

  virtual ~View() = default;

  inline void setId(ViewId id) { _id = id; }

  [[nodiscard]] inline ViewId id() const { return _id; };

  [[nodiscard]] inline VarViewId parentId() const { return _parentId; }
};

}  // namespace atlantis::propagation
