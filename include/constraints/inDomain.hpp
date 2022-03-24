#pragma once

#include <cassert>

#include "constraint.hpp"
#include "core/types.hpp"
#include "variables/intVar.hpp"

class Engine;
class InDomain : public Constraint {
 private:
  const std::vector<DomainEntry> _domain;
  const VarId _x;

 public:
  InDomain(VarId violationId, VarId x, std::vector<DomainEntry> domain);

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};
