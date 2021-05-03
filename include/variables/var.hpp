#pragma once
#include "core/types.hpp"

class Var {
 protected:
  VarId m_id;

 public:
  explicit Var(VarId t_id);
  ~Var() = default;
};