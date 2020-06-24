#pragma once

#include <memory>
#include <vector>
#include <limits>

#include "../core/constraint.hpp"
#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/tracer.hpp"
#include "../core/types.hpp"

class AllDifferent : public Constraint {
 private:
  std::vector<VarId> m_variables;
  std::vector<SavedInt> m_overlaps;
  Int m_lowerBound;
  Int m_upperBound;
  Int getOverlap(Timestamp ts, Int overlappingValue);
  void setOverlap(Timestamp ts, Int overlappingValue, Int newOverlap);
  void increaseOverlap(Timestamp ts, Int overlappingValue, Int delta);
 public:
  AllDifferent(VarId violationId, std::vector<VarId>&& t_variables);
  
  ~AllDifferent() = default;
  virtual void init(Timestamp, Engine&) override;
  virtual void recompute(Timestamp, Engine&) override;
  virtual void notifyIntChanged(Timestamp t, Engine& e, LocalId id,
                                Int oldValue, Int newValue, Int data) override;
  virtual void commit(Timestamp, Engine&) override;
  virtual VarId getNextDependency(Timestamp, Engine& e) override;
  virtual void notifyCurrentDependencyChanged(Timestamp, Engine& e) override;
};
