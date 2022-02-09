#pragma once

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;  // Forward declaration

class Constraint : public Invariant {
 private:
 protected:
  const VarId _violationId;
  Constraint(Id id, VarId violationId)
      : Invariant(id), _violationId(violationId) {}

 public:
  inline VarId getViolationId() const;
  inline Int getViolationCount(Engine&, Timestamp&) const;
};