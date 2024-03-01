#pragma once

#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/variables/var.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class SolverBase;

class View : public Var {
 protected:
  SolverBase& _solver;
  VarId _parentId;

 public:
  explicit View(SolverBase& solver, VarId parentId)
      : Var(NULL_ID), _solver(solver), _parentId(parentId) {
    _id.idType = VarIdType::view;
  }
  virtual ~View() = default;
  inline void setId(const VarId id) { _id.id = id.id; }
  [[nodiscard]] inline VarId id() const { return _id; };
  [[nodiscard]] inline VarId parentId() const { return _parentId; }
};

}  // namespace atlantis::propagation
