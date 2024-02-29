#pragma once
#include "atlantis/propagation/types.hpp"

namespace atlantis::propagation {

class Var {
 protected:
  VarId _id;

 public:
  explicit Var(VarId);
  ~Var() = default;
};

}  // namespace atlantis::propagation
