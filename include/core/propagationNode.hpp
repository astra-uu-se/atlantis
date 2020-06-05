#pragma once
#include "core/types.hpp"

class PropagationNode {
 public:
  bool m_isInvalid;
  Id m_id;
  PropagationNode() = delete;
  PropagationNode(Id t_id) : m_isInvalid(false), m_id(t_id){};
  ~PropagationNode() = default;
  void setId(Id t_id) { m_id = t_id; }
  inline void invalidate([[maybe_unused]]const Timestamp& t) { m_isInvalid = true; }
  inline void validate([[maybe_unused]]const Timestamp& t) { m_isInvalid = false; }
};
