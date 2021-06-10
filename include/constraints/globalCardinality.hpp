#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

#include "core/types.hpp"
#include "variables/intVar.hpp"
// #include "variables/savedInt.hpp"
#include "constraint.hpp"

class SavedInt;  // forward declare
class Engine;

class GlobalCardinality : public Constraint {
 private:
  std::vector<VarId> m_variables;
  std::vector<Int> m_cover;
  std::vector<Int> m_lowerBound;
  std::vector<Int> m_upperBound;
  std::vector<SavedInt> m_localValues;
  SavedInt m_excess;
  SavedInt m_shortage;
  std::vector<SavedInt> m_counts;
  Int m_offset;
  Int m_closed;  // closed = 0 designates that GCC is closed, else 1
  signed char increaseCount(Timestamp ts, Int value);
  signed char decreaseCount(Timestamp ts, Int value);

 public:
  GlobalCardinality(VarId violationId, std::vector<VarId> t_variables,
                    std::vector<Int> cover, std::vector<Int> t_counts);
  GlobalCardinality(VarId violationId, std::vector<VarId> t_variables,
                    std::vector<Int> cover, std::vector<Int> t_counts,
                    bool closed);
  GlobalCardinality(VarId violationId, std::vector<VarId> t_variables,
                    std::vector<Int> cover, std::vector<Int> lowerBound,
                    std::vector<Int> upperBound);
  GlobalCardinality(VarId violationId, std::vector<VarId> t_variables,
                    std::vector<Int> cover, std::vector<Int> lowerBound,
                    std::vector<Int> upperBound, bool closed);

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp t, Engine& e, LocalId id) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextDependency(Timestamp, Engine& e) override;
  void notifyCurrentDependencyChanged(Timestamp, Engine& e) override;
};

inline signed char GlobalCardinality::increaseCount(Timestamp ts, Int value) {
  size_t pos = static_cast<size_t>(std::max<Int>(
      0, std::min(Int(m_lowerBound.size()) - 1, value - m_offset)));
  Int newCount = m_counts.at(pos).incValue(ts, 1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(m_variables.size()));
  return m_lowerBound.at(pos) < 0
             ? 0
             : (newCount > m_upperBound.at(pos)
                    ? 1
                    : (newCount > m_lowerBound.at(pos) ? 0 : -1));
}

inline signed char GlobalCardinality::decreaseCount(Timestamp ts, Int value) {
  size_t pos = static_cast<size_t>(std::max<Int>(
      0, std::min(Int(m_lowerBound.size()) - 1, value - m_offset)));
  Int newCount = m_counts.at(pos).incValue(ts, -1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(m_variables.size()));
  return m_lowerBound.at(pos) < 0
             ? 0
             : (newCount < m_lowerBound.at(pos)
                    ? 1
                    : (newCount < m_upperBound.at(pos) ? 0 : -1));
}