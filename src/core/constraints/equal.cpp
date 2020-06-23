#include "constraints/equal.hpp"

// TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;

/**
 * Constraint a*x = b*y
 * @param violationId id for the violationCount
 * @param a coefficient of x
 * @param x variable of lhs
 * @param b coefficient of y
 * @param y variable of rhs
 */
Equal::Equal(VarId violationId, VarId x, VarId y)
    : Constraint(NULL_ID, violationId), m_x(x), m_y(y) {}

void Equal::init([[maybe_unused]] const Timestamp& t, Engine& e) {
  assert(m_id != NULL_ID);

  e.registerInvariantDependsOnVar(m_id, m_x, LocalId(m_x), 0);
  e.registerInvariantDependsOnVar(m_id, m_y, LocalId(m_y), 0);
  e.registerDefinedVariable(m_violationId, m_id);
}

void Equal::recompute(const Timestamp& t, Engine& e) {
  e.setValue(t, m_violationId,
             std::abs(e.getValue(t, m_x) - e.getValue(t, m_y)));
}

void Equal::notifyIntChanged(const Timestamp& t, Engine& e,
                             [[maybe_unused]] LocalId id, Int oldValue,
                             Int newValue, [[maybe_unused]] Int coef) {
  assert(newValue != oldValue);  // precondition
  e.setValue(t, m_violationId,
             std::abs(e.getValue(t, m_x) - e.getValue(t, m_y)));
}

VarId Equal::getNextDependency(const Timestamp& t) {
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

void Equal::notifyCurrentDependencyChanged(const Timestamp& t, Engine& e,
                                           Int oldValue, Int newValue) {
  assert(m_state.getValue(t) != -1);
  assert(newValue != oldValue);
  e.setValue(t, m_violationId,
             std::abs(e.getValue(t, m_x) - e.getValue(t, m_y)));
}

void Equal::commit(const Timestamp& t, Engine& e) {
  // todo: do nodes validate themself or is it done by engine?
  // this->validate(t);
  e.commitIf(t, m_violationId);
}