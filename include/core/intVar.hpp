#pragma once

#include "SavedInt.hpp"
#include "Var.hpp"

class IntVar : public Var {
 private: 
  SavedInt m_value;

 public:
  IntVar() = delete;
  IntVar(Int t_id);
  ~IntVar();
};