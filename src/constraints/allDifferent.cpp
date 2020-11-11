#include "constraints/allDifferent.hpp"

// TODO: invariant should take its true id in the constructor.

/**
 * @param violationId id for the violationCount
 */
AllDifferent::AllDifferent(VarId violationId, std::vector<VarId> t_variables)
    : Constraint(NULL_ID, violationId),
      m_variables(std::move(t_variables)),
      m_localValues(),
      m_counts(),
      m_offset(0){
  m_modifiedVars.resize(m_variables.size(),false);
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

  e.updateValue(t, m_violationId, 0);

  for (size_t i = 0; i < m_variables.size(); ++i) {
    increaseCount(t, e, e.getValue(t, m_variables[i]));
    m_localValues[i].setValue(t, e.getValue(t, m_variables[i]));
  }
}

void AllDifferent::notifyIntChanged(Timestamp t, Engine& e, LocalId id) {
  Int oldValue = m_localValues.at(id).getValue(t);
  auto newValue = e.getValue(t, m_variables[id]);
  if (newValue == oldValue) {
    return;
  }
  decreaseCount(t, e, oldValue);
  increaseCount(t, e, newValue);
  m_localValues.at(id).setValue(t, newValue);
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
  auto index = static_cast<size_t>(m_state.getValue(t));
  assert(index < m_variables.size());

  VarId varId = m_variables.at(index);

  Int oldValue = m_localValues.at(index).getValue(t);
  Int newValue = e.getNewValue(varId);

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
