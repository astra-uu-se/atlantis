#pragma once

#include "atlantis/propagation/variables/var.hpp"

namespace atlantis::propagation {

class SolverBase;

class View : public Var {
 protected:
  SolverBase& _solver;
  VarViewId _parentId;

 public:
  explicit View(SolverBase& solver, VarViewId parentId);

  void setId(ViewId id);

  [[nodiscard]] ViewId id() const { return _id; }

  [[nodiscard]] VarViewId parentId() const { return _parentId; }
};

inline View::View(SolverBase& solver, const VarViewId parentId)
    : Var(NULL_ID), _solver(solver), _parentId(parentId) {}

inline void View::setId(const ViewId id) { _id = id; }

}  // namespace atlantis::propagation
