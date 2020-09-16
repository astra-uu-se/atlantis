#pragma once
#include "core/types.hpp"

class Var {
 protected:
  VarId m_id;

 public:
  Var(Int t_id);
  ~Var() = default;
  VarId getId();
};

inline VarId Var::getId() { return m_id; }