#include "constraints/lessThan.hpp"

#include "core/engine.hpp"

/**
 * Constraint x < y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
LessThan::LessThan(VarId violationId, VarId x, VarId y)
    : Constraint(NULL_ID, violationId), m_x(x), m_y(y) {
  m_modifiedVars.reserve(1);
}

void LessThan::init(Timestamp, Engine& e) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(m_id != NULL_ID);

  e.registerInvariantDependsOnVar(m_id, m_x, LocalId(0));
  e.registerInvariantDependsOnVar(m_id, m_y, LocalId(0));
  registerDefinedVariable(e, m_violationId);
}

void LessThan::recompute(Timestamp t, Engine& e) {
  // Dereference safe as incValue does not retain ptr.
  updateValue(t, e, m_violationId,
              std::max((Int)0, e.getValue(t, m_x) - e.getValue(t, m_y) + 1));
}

void LessThan::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  recompute(t, e);
}

VarId LessThan::getNextDependency(Timestamp t, Engine&) {
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

void LessThan::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  recompute(t, e);
}

void LessThan::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }
