#include "constraints/equal.hpp"

// TODO: invariant should take its true id in the constructor.

/**
 * Constraint a*x = b*y
 * @param violationId id for the violationCount
 * @param a coefficient of x
 * @param x variable of lhs
 * @param b coefficient of y
 * @param y variable of rhs
 */
Equal::Equal(VarId violationId, VarId x, VarId y)
    : Constraint(NULL_ID, violationId), m_x(x), m_y(y) {
  //  m_modifiedVars.resize(1,false);
  m_modifiedVars.reserve(1);
}

void Equal::init(Timestamp, Engine& e) {
  assert(m_id != NULL_ID);

  e.registerInvariantDependsOnVar(m_id, m_x, LocalId(0));
  e.registerInvariantDependsOnVar(m_id, m_y, LocalId(0));
  registerDefinedVariable(e, m_violationId);
}

void Equal::recompute(Timestamp t, Engine& e) {
  updateValue(t, e, m_violationId,
              std::abs(e.getValue(t, m_x) - e.getValue(t, m_y)));
}

void Equal::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  updateValue(t, e, m_violationId,
              std::abs(e.getValue(t, m_x) - e.getValue(t, m_y)));
}

VarId Equal::getNextDependency(Timestamp t, Engine&) {
  m_state.incValue(t, 1);
  // todo: maybe this can be faster by first checking null and then doing
  // ==0?m_x:m_y;
  switch (m_state.getValue(t)) {
    case 0:
      return m_x;
    case 1:
      return m_y;
    default:
      return NULL_ID;
  }
}

void Equal::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  assert(m_state.getValue(t) != -1);
  // assert(newValue != oldValue);
  updateValue(t, e, m_violationId,
              std::abs(e.getValue(t, m_x) - e.getValue(t, m_y)));
}

void Equal::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }
