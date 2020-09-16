#pragma once
#include "core/types.hpp"
#include "core/var.hpp"

class VarView : Var {
 protected:
  VarId m_sourceId;

 public:
  VarView(const VarId& t_sourceId);
  ~VarView() = default;
  inline void setId(const VarId& t_id) { m_id = t_id; }
  inline VarId getId() const { return m_id; };
  inline VarId getSourceId() const { return m_sourceId; }
};