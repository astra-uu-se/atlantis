#pragma once
#include "core/propagationNode.hpp"
#include "core/types.hpp"

class Var : public PropagationNode {
 protected:
 public:
  Var(Int t_id);
  ~Var() = default;
};