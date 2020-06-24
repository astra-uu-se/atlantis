#include "constraints/allDifferent.hpp"

// TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;

/**
 * @param violationId id for the violationCount
 */
AllDifferent::AllDifferent(VarId violationId, std::vector<VarId>&& t_variables)
    : Constraint(NULL_ID, violationId),
      m_variables(std::move(t_variables)),
      m_overlaps(),
      m_lowerBound(0),
      m_upperBound(0) {}

void AllDifferent::init(Timestamp ts, Engine& e) {
  assert(m_id != NULL_ID);
  m_lowerBound = std::numeric_limits<Int>::max();
  Int upperBound = std::numeric_limits<Int>::min();

  for (VarId varId : m_variables) {
    e.registerInvariantDependsOnVar(m_id, varId, LocalId(varId), 0);
    m_lowerBound = std::min(m_lowerBound, e.getLowerBound(varId));
    upperBound = std::max(upperBound, e.getUpperBound(varId));
  }
  m_overlaps.reserve(upperBound - m_lowerBound);
  for (Int i = 0; i < upperBound - m_lowerBound; ++i) {
    m_overlaps.emplace_back(ts, 0);
  }

  e.registerDefinedVariable(m_violationId, m_id);
}

inline Int AllDifferent::getOverlap(Timestamp ts, Int overlappingValue) {
  return m_overlaps.at(overlappingValue - m_lowerBound).getValue(ts);
}

inline void AllDifferent::setOverlap(Timestamp ts, Int overlappingValue, Int newOverlap) {
  m_overlaps.at(overlappingValue - m_lowerBound).setValue(ts, newOverlap);
}

inline void AllDifferent::increaseOverlap(Timestamp ts, Int overlappingValue, Int delta) {
  setOverlap(ts, overlappingValue, getOverlap(ts, overlappingValue) + delta);
}

void AllDifferent::recompute(Timestamp t, Engine& e) {
  for (auto varId : m_variables) {
    setOverlap(t, e.getValue(t, varId), 0);
  }
  Int violation = 0;
  for (auto varId : m_variables) {
    if (getOverlap(t, e.getValue(t, varId)) > 0) {
      ++violation;
    }
    increaseOverlap(t, e.getValue(t, varId), 1);
  }
  e.setValue(t, m_violationId, violation);
}

void AllDifferent::notifyIntChanged(Timestamp t, Engine& e,
                             LocalId, Int oldValue,
                             Int newValue, Int) {
  increaseOverlap(t, oldValue, -1);
  increaseOverlap(t, newValue, 1);
  Int delta = (
    (getOverlap(t, newValue) > 1 ? 1 : 0) -
    (getOverlap(t, oldValue) > 0 ? 1 : 0)
  );
  if (delta == 0) {
    return;
  }
  
  e.incValue(t, m_violationId, delta);
}

VarId AllDifferent::getNextDependency(Timestamp t, Engine&) {
  m_state.incValue(t, 1);

  Int index = m_state.getValue(t);
  if (0 <= index && index < (Int) m_variables.size()) {
    return m_variables[index];
  }
  return NULL_ID;
}

void AllDifferent::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  auto index = m_state.getValue(t);
  assert(0 <= index);
  assert(index < (Int) m_variables.size());
  
  VarId varId = m_variables.at(index);

  Int oldValue = e.getCommitedValue(varId);
  Int newValue = e.getValue(varId);

  increaseOverlap(t, oldValue, -1);
  increaseOverlap(t, newValue, 1);
  Int delta = (
    (getOverlap(t, newValue) > 1 ? 1 : 0) -
    (getOverlap(t, oldValue) > 0 ? 1 : 0)
  );
  if (delta == 0) {
    return;
  }
  
  e.incValue(t, m_violationId, delta);
}

void AllDifferent::commit(Timestamp t, Engine& e) {
  for (SavedInt savedInt : m_overlaps) {
    savedInt.commitIf(t);
  }
  e.commitIf(t, m_violationId);
}