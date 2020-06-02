#pragma once
#include "core/types.hpp"

class PropagationNode {
 public:
  bool m_isInvalid;
  Id m_id;
  PropagationNode(Id t_id) : m_id(t_id), m_isInvalid(false){};
  ~PropagationNode();
  void setId(Id t_id) { m_id = t_id; }
  inline void invalidate() { m_isInvalid = true; }
};
