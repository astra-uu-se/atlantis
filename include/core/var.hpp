#pragma once
#include "core/propagationElement.hpp"
#include "core/types.hpp"

class Var : public PropagationElement {
 protected:
 public:
  Var(Int t_id);
  ~Var();
};