#pragma once
#include "core/types.hpp"
#include "variables/var.hpp"

class View : public Var {
 protected:
  const VarId _parentId;

 public:
  explicit View(VarId parentId) : Var(NULL_ID), _parentId(parentId) {
    _id.idType = VarIdType::view;
  }
  virtual ~View() = default;
  inline void setId(const VarId id) { _id.id = id.id; }
  [[nodiscard]] inline VarId id() const { return _id; };
  [[nodiscard]] inline VarId parentId() const { return _parentId; }
};