#pragma once
#include "core/types.hpp"

class Var {
 protected:
  VarId m_id;

 public:
  Var(Int t_id);
  ~Var() = default;
};