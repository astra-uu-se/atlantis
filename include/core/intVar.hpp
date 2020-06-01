#pragma once

#include "core/savedInt.hpp"
#include "core/var.hpp"

class IntVar : public Var {
 private: 
  SavedInt m_value;

 public:
  IntVar() = delete;
  IntVar(Int t_id);
  ~IntVar();
};