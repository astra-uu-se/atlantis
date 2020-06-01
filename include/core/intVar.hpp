#pragma once

#include "core/savedInt.hpp"
#include "core/var.hpp"

class IntVar : public Var {
 private:
  SavedInt m_value;

 public:
  IntVar() = delete;
  IntVar(Id t_id);
  ~IntVar();

  Int getNewValue(Timestamp& t) { return m_value.getValue(t); }
  Int getOldValue() { return m_value.getCommittedValue(); }
};