#include "constraints/lessEqual.hpp"

// TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;

/**
 * Constraint a*x <= b*y
 * @param violationId id for the violationCount
 * @param a coefficient of x
 * @param x variable of lhs
 * @param b coefficient of y
 * @param y variable of rhs
 */
LessEqual::LessEqual(VarId violationId, VarId x, VarId y)
    : Constraint(NULL_ID, violationId), m_x(x), m_y(y) {}

void LessEqual::init(Timestamp, Engine& e) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(m_id != NULL_ID);

  e.registerInvariantDependsOnVar(m_id, m_x, LocalId(m_x));
  e.registerInvariantDependsOnVar(m_id, m_y, LocalId(m_y));
  e.registerDefinedVariable(m_violationId, m_id);
}

void LessEqual::recompute(Timestamp t, Engine& e) {
  // Dereference safe as incValue does not retain ptr.
  e.setValue(t, m_violationId,
             std::max((Int)0, e.getValue(t, m_x) - e.getValue(t, m_y)));
}

void LessEqual::notifyIntChanged(Timestamp t, Engine& e, LocalId, Int) {
  // if x decreases and violation is 0, then do nothing
  // if y increases and violation is 0, then do nothing
  e.setValue(t, m_violationId,
             std::max((Int)0, e.getValue(t, m_x) - e.getValue(t, m_y)));
}

VarId LessEqual::getNextDependency(Timestamp t, Engine&) {
  m_state.incValue(t, 1);
  // todo: maybe this can be faster by first checking null and then doing
  // ==0?m_x:m_y;
  switch (m_state.getValue(t)) {
    case 0:
      return m_x;
      break;
    case 1:
      return m_y;
      break;
    default:
      return NULL_ID;
  }
}

void LessEqual::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  assert(m_state.getValue(t) != -1);
  // assert(newValue != oldValue);
  e.setValue(t, m_violationId,
             std::max((Int)0, e.getValue(t, m_x) - e.getValue(t, m_y)));
}

void LessEqual::commit(Timestamp t, Engine& e) {
  Invariant::commit(t,e);
}