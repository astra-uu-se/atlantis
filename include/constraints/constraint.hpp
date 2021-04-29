#pragma once
#include "core/invariant.hpp"
#include "core/types.hpp"

class Engine;  // Forward declaration

class Constraint : public Invariant {
 private:
 protected:
  VarId m_violationId;
  Constraint(Id t_id, VarId t_violationId)
      : Invariant(t_id), m_violationId(t_violationId) {}

 public:
  inline VarId getViolationId();
  inline Int getViolationCount(Engine& e, Timestamp& t);
};