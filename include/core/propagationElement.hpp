#pragma once
#include "core/types.hpp"

class PropagationElement {
 protected:

 public:
  Id m_id;
  PropagationElement(Id t_id);
  ~PropagationElement();
  void setId(Id t_id) { m_id = t_id; }
};
