#pragma once

#include <limits>
#include <memory>
#include <vector>

#include "../core/constraint.hpp"
#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/savedInt.hpp"
#include "../core/tracer.hpp"
#include "../core/types.hpp"

class AllDifferent : public Constraint {
 private:
  std::vector<VarId> m_variables;
  std::vector<SavedInt> m_localValues;
  std::vector<SavedInt> m_counts;
  Int m_offset;
  signed char increaseCount(Timestamp ts, Int value);
  signed char decreaseCount(Timestamp ts, Int value);

 public:
  AllDifferent(VarId violationId, std::vector<VarId> t_variables);

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp t, Engine& e, LocalId id) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextDependency(Timestamp, Engine& e) override;
  void notifyCurrentDependencyChanged(Timestamp, Engine& e) override;
};

inline signed char AllDifferent::increaseCount(Timestamp ts, Int value) {
  Int newCount = m_counts.at(value - m_offset).incValue(ts, 1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(m_variables.size()));
//  if (newCount >= 2) {
//    incValue(ts, e, m_violationId, 1);
//  }
  return newCount >= 2 ? 1:0;
}

inline signed char AllDifferent::decreaseCount(Timestamp ts, Int value) {
  Int newCount = m_counts.at(value - m_offset).incValue(ts, -1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(m_variables.size()));
//  if (newCount >= 1) {
//    incValue(ts, e, m_violationId, -1);
//  }
  return newCount >= 1 ? -1:0;
}
