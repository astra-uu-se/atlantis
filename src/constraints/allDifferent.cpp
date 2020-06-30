#include "constraints/allDifferent.hpp"

// TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;

/**
 * @param violationId id for the violationCount
 */
AllDifferent::AllDifferent(VarId violationId, std::vector<VarId> t_variables)
    : Constraint(NULL_ID, violationId),
      m_variables(t_variables),
      m_counts(),
      m_offset(0)
       {}

void AllDifferent::init(Timestamp ts, Engine& e) {
  assert(m_id != NULL_ID);
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (size_t i = 0; i < m_variables.size(); ++i) {
    lb = std::min(lb, e.getLowerBound(m_variables[i]));
    ub = std::max(ub, e.getUpperBound(m_variables[i]));
    e.registerInvariantDependsOnVar(m_id, m_variables[i], LocalId(i), 0);
  }
  assert(ub >= lb);

  m_counts.resize(ub-lb+1, SavedInt(ts, 0));

  e.registerDefinedVariable(m_violationId, m_id);
  
  m_offset = lb;
}

inline void AllDifferent::increaseCount(Timestamp ts, Engine& e, Int value) {
  Int newCount = m_counts.at(value-m_offset).incValue(ts,1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(m_variables.size()));
  if(newCount >= 2){
    e.incValue(ts, m_violationId, 1);
  }
}

inline void AllDifferent::decreaseCount(Timestamp ts, Engine& e, Int value) {
  Int newCount = m_counts.at(value-m_offset).incValue(ts, -1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(m_variables.size()));
  if(newCount >= 1){
    e.incValue(ts, m_violationId, -1);
  }
}

void AllDifferent::recompute(Timestamp t, Engine& e) {
  for(SavedInt& c: m_counts){
    c.setValue(t, 0);
  }
  
  e.setValue(t, m_violationId, 0);
  
  for (auto varId : m_variables) {
    increaseCount(t, e, e.getValue(t,varId));
  }
}

void AllDifferent::notifyIntChanged(Timestamp t, Engine& e,
                             LocalId, Int oldValue,
                             Int newValue, Int) {
  assert(newValue != oldValue);
  decreaseCount(t, e, oldValue);
  increaseCount(t, e, newValue);
}

VarId AllDifferent::getNextDependency(Timestamp t, Engine&) {
  m_state.incValue(t, 1);

  Int index = m_state.getValue(t);
  if (0 <= index && index < static_cast<Int>(m_variables.size())) {
    return m_variables.at(index);
  }
  return NULL_ID;
}

void AllDifferent::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  auto index = m_state.getValue(t);
  assert(0 <= index);
  assert(index < static_cast<Int>(m_variables.size()));
  
  VarId varId = m_variables.at(index);

  Int oldValue = e.getCommittedValue(varId);
  Int newValue = e.getValue(varId);

  decreaseCount(t, e, oldValue);
  increaseCount(t, e, newValue);
}

void AllDifferent::commit(Timestamp t, Engine&) {
  for (SavedInt& savedInt : m_counts) {
    savedInt.commitIf(t);
  }
}