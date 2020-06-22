#include "constraints/lessEqual.hpp"

//TODO: invariant should take its true id in the constructor.
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
  : Constraint(NULL_ID, violationId),
    m_x(x),
    m_y(y) {
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "constructing constraint"
            << "\n";
#endif
}

// LessEqual::LessEqual(Engine& e, std::vector<Int>&& A,
//                std::vector<std::shared_ptr<IntVar>>&& X,
//                std::shared_ptr<IntVar> b)
//     : Invariant(Engine::NULL_ID), m_A(std::move(A)), m_X(std::move(X)),
//     m_b(b) {
//   init(e);
// }

void LessEqual::init([[maybe_unused]] const Timestamp& t, Engine& e) {
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "initialising invariant " << m_id << "\n";
#endif
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(m_id != NULL_ID);
  
  e.registerInvariantDependsOnVar(m_id, m_x, LocalId(m_x), 0);
  e.registerInvariantDependsOnVar(m_id, m_y, LocalId(m_y), 0);
  e.registerDefinedVariable(m_violationId, m_id);
}

void LessEqual::recompute(const Timestamp& t, Engine& e) {
  
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Constraint LessEqual[" << m_id
            << "] with violation " << std::max((Int) 0, e.getValue(t, m_x)) << "\n" - e.getValue(t, m_y);
#endif
  // Dereference safe as incValue does not retain ptr.
  e.setValue(
    t,
    m_violationId,
    std::max((Int) 0, e.getValue(t, m_x) - e.getValue(t, m_y))
  );
}

void LessEqual::notifyIntChanged(const Timestamp& t, Engine& e,
                              [[maybe_unused]] LocalId id, Int oldValue,
                              Int newValue, [[maybe_unused]] Int coef) {
  assert(newValue != oldValue);  // precondition
  // if x decreases and violation is 0, then do nothing
  // if y increases and violation is 0, then do nothing
  e.setValue(
    t,
    m_violationId,
    std::max((Int) 0, e.getValue(t, m_x) - e.getValue(t, m_y))
  );

#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Constraint LessEqual[" << m_id << "] notifying output changed to "
            << std::max((Int) 0, e.getValue(t, m_x)) << "\n" - e.getValue(t, m_y);
#endif
}

void LessEqual::commit(const Timestamp& t, Engine& e) {
  // todo: do nodes validate themself or is it done by engine?
  // this->validate(t);
  e.commitIf(t, m_violationId);
}