#pragma once
#include "core/types.hpp"

class Var {
 protected:
  VarId _id;

 public:
  explicit Var(VarId);
  ~Var() = default;
};