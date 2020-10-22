#include "constraints/allDifferent.hpp"

// TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;

/**
 * @param violationId id for the violationCount
 */
AllDifferent::AllDifferent(VarId violationId, std::vector<VarId> t_variables)
    : Constraint(NULL_ID, violationId),
      m_variables(t_variables),
      m_localValues(),
      m_counts(),
      m_offset(0) {}

void AllDifferent::init(Timestamp ts, Engine& e) {
  assert(m_id != NULL_ID);
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (size_t i = 0; i < m_variables.size(); ++i) {
    lb = std::min(lb, e.getLowerBound(m_variables[i]));
    ub = std::max(ub, e.getUpperBound(m_variables[i]));
    e.registerInvariantDependsOnVar(m_id, m_variables[i], LocalId(i));
    m_localValues.push_back(SavedInt(ts, e.getCommittedValue(m_variables[i])));
  }
  assert(ub >= lb);

  m_counts.resize(ub - lb + 1, SavedInt(ts, 0));

  e.registerDefinedVariable(m_violationId, m_id);

  m_offset = lb;
}

inline void AllDifferent::increaseCount(Timestamp ts, Engine& e, Int value) {
  Int newCount = m_counts.at(value - m_offset).incValue(ts, 1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(m_variables.size()));
  if (newCount >= 2) {
    e.incValue(ts, m_violationId, 1);
  }
}

inline void AllDifferent::decreaseCount(Timestamp ts, Engine& e, Int value) {
  Int newCount = m_counts.at(value - m_offset).incValue(ts, -1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(m_variables.size()));
  if (newCount >= 1) {
    e.incValue(ts, m_violationId, -1);
  }
}

void AllDifferent::recompute(Timestamp t, Engine& e) {
  for (SavedInt& c : m_counts) {
    c.setValue(t, 0);
  }

  e.updateValue(t, m_violationId, 0);

  for (size_t i = 0; i < m_variables.size(); ++i) {
    increaseCount(t, e, e.getValue(t, m_variables[i]));
    m_localValues[i].setValue(t, e.getValue(t, m_variables[i]));
  }
}

void AllDifferent::notifyIntChanged(Timestamp t, Engine& e, LocalId id,
                                    Int newValue) {
  Int oldValue = m_localValues.at(id).getValue(t);
  // assert(newValue != oldValue);  // Actually this assert is wrong and should
  // be replaced by an if(...) return; Only keeping it becuase it helps catch
  // bugs before we have an example with cycles.
  decreaseCount(t, e, oldValue);
  increaseCount(t, e, newValue);
  m_localValues.at(id).setValue(t, newValue);
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

  Int oldValue = m_localValues.at(index).getValue(t);
  Int newValue = e.getValue(varId);

  decreaseCount(t, e, oldValue);
  increaseCount(t, e, newValue);

  m_localValues.at(index).setValue(t, newValue);
}

void AllDifferent::commit(Timestamp t, Engine& e) {
  Invariant::commit(t, e);

  for (auto& localValue : m_localValues) {
    localValue.commitIf(t);
  }

  for (SavedInt& savedInt : m_counts) {
    savedInt.commitIf(t);
  }
}
