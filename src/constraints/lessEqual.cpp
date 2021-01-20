#include "constraints/lessEqual.hpp"

/**
 * Constraint a*x <= b*y
 * @param violationId id for the violationCount
 * @param a coefficient of x
 * @param x variable of lhs
 * @param b coefficient of y
 * @param y variable of rhs
 */
LessEqual::LessEqual(VarId violationId, VarId x, VarId y)
    : Constraint(NULL_ID, violationId), m_x(x), m_y(y) {
  m_modifiedVars.reserve(1);
}

void LessEqual::init(Timestamp, Engine& e) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(m_id != NULL_ID);

  e.registerInvariantDependsOnVar(m_id, m_x, LocalId(0));
  e.registerInvariantDependsOnVar(m_id, m_y, LocalId(0));
  registerDefinedVariable(e, m_violationId);
}

void LessEqual::recompute(Timestamp t, Engine& e) {
  // Dereference safe as incValue does not retain ptr.
  updateValue(t, e, m_violationId,
              std::max((Int)0, e.getValue(t, m_x) - e.getValue(t, m_y)));
}

void LessEqual::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  // if x decreases and violation is 0, then do nothing
  // if y increases and violation is 0, then do nothing
  updateValue(t, e, m_violationId,
              std::max((Int)0, e.getValue(t, m_x) - e.getValue(t, m_y)));
}

VarId LessEqual::getNextDependency(Timestamp t, Engine&) {
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

void LessEqual::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  assert(m_state.getValue(t) != -1);
  updateValue(t, e, m_violationId,
              std::max((Int)0, e.getValue(t, m_x) - e.getValue(t, m_y)));
}

void LessEqual::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }
