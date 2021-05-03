#pragma once
#include "core/types.hpp"
#include "variables/var.hpp"

class View : public Var {
 protected:
  VarId m_parentId;

 public:
  explicit View(VarId t_parentId) : Var(NULL_ID), m_parentId(t_parentId) {
    m_id.idType = VarIdType::view;
  }
  virtual ~View() = default;
  inline void setId(const VarId t_id) { m_id.id = t_id.id; }
  [[nodiscard]] inline VarId getId() const { return m_id; };
  [[nodiscard]] inline VarId getParentId() const { return m_parentId; }
};