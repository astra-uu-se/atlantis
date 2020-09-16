#pragma once
#include "core/types.hpp"
#include "core/var.hpp"

class VarView : Var {
 protected:
  VarId m_parentId;

 public:
  VarView(const VarId& t_parentId);
  ~VarView() = default;
  inline void setId(const VarId& t_id) { m_id.id = t_id.id; }
  inline VarId getId() const { return m_id; };
  inline VarId getParentId() const { return m_parentId; }
};