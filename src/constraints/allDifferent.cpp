#include "constraints/allDifferent.hpp"

/**
 * @param violationId id for the violationCount
 */
AllDifferent::AllDifferent(VarId violationId, std::vector<VarId> t_variables)
    : Constraint(NULL_ID, violationId),
      m_variables(std::move(t_variables)),
      m_localValues(),
      m_counts(),
      m_offset(0) {
  m_modifiedVars.reserve(m_variables.size());
}

void AllDifferent::init(Timestamp ts, Engine& e) {
  assert(!m_id.equals(NULL_ID));
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (size_t i = 0; i < m_variables.size(); ++i) {
    lb = std::min(lb, e.getLowerBound(m_variables[i]));
    ub = std::max(ub, e.getUpperBound(m_variables[i]));
    e.registerInvariantDependsOnVar(m_id, m_variables[i], LocalId(i));
    m_localValues.emplace_back(ts, e.getCommittedValue(m_variables[i]));
  }
  assert(ub >= lb);

  m_counts.resize(static_cast<unsigned long>(ub - lb + 1), SavedInt(ts, 0));

  registerDefinedVariable(e, m_violationId);

  m_offset = lb;
}

void AllDifferent::recompute(Timestamp t, Engine& e) {
  for (SavedInt& c : m_counts) {
    c.setValue(t, 0);
  }

  updateValue(t, e, m_violationId, 0);

  Int violInc = 0;
  for (size_t i = 0; i < m_variables.size(); ++i) {
    violInc += increaseCount(t, e.getValue(t, m_variables[i]));
    m_localValues[i].setValue(t, e.getValue(t, m_variables[i]));
  }
  incValue(t, e, m_violationId, violInc);
}

void AllDifferent::notifyIntChanged(Timestamp t, Engine& e, LocalId id) {
  Int oldValue = m_localValues.at(id).getValue(t);
  auto newValue = e.getValue(t, m_variables[id]);
  if (newValue == oldValue) {
    return;
  }
  signed char dec = decreaseCount(t, oldValue);
  signed char inc = increaseCount(t, newValue);
  m_localValues.at(id).setValue(t, newValue);
  incValue(t, e, m_violationId, static_cast<Int>(dec + inc));
}

VarId AllDifferent::getNextDependency(Timestamp t, Engine&) {
  m_state.incValue(t, 1);

  auto index = static_cast<size_t>(m_state.getValue(t));
  if (index < m_variables.size()) {
    return m_variables.at(index);
  }
  return NULL_ID;
}

void AllDifferent::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  auto id = static_cast<size_t>(m_state.getValue(t));
  assert(id < m_variables.size());
  notifyIntChanged(t, e, id);
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
