#include "constraints/equal.hpp"

//TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;


/**
 * Constraint a*x = b*y
 * @param violationId id for the violationCount
 * @param a coefficient of x
 * @param x variable of lhs
 * @param b coefficient of y
 * @param y variable of rhs
 */
Equal::Equal(VarId violationId, Int a, VarId x, Int b, VarId y)
  : Constraint(NULL_ID, violationId),
    m_a(a),
    m_x(x),
    m_b(b),
    m_y(y) {
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "constructing constraint"
            << "\n";
#endif
}

// Equal::Equal(Engine& e, std::vector<Int>&& A,
//                std::vector<std::shared_ptr<IntVar>>&& X,
//                std::shared_ptr<IntVar> b)
//     : Invariant(Engine::NULL_ID), m_A(std::move(A)), m_X(std::move(X)),
//     m_b(b) {
//   init(e);
// }

void Equal::init([[maybe_unused]] const Timestamp& t, Engine& e) {
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "initialising invariant " << m_id << "\n";
#endif
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(m_id != NULL_ID);
  
  e.registerInvariantDependsOnVar(m_id, m_x, LocalId(m_x), m_a);
  e.registerInvariantDependsOnVar(m_id, m_y, LocalId(m_y), m_b);
  e.registerDefinedVariable(m_violationId, m_id);
}

void Equal::recompute(const Timestamp& t, Engine& e) {
  
#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Constraint Equal[" << m_id
            << "] with violation " << std::abs(m_a * e.getValue(t, m_x) - m_b * e.getValue(t, m_y)) << "\n";
#endif
  // Dereference safe as incValue does not retain ptr.
  e.setValue(
    t,
    m_violationId,
    std::abs(m_a * e.getValue(t, m_x) - m_b * e.getValue(t, m_y))
  );
}

void Equal::notifyIntChanged(const Timestamp& t, Engine& e,
                              [[maybe_unused]] LocalId id, Int oldValue,
                              Int newValue, [[maybe_unused]] Int coef) {
  assert(newValue != oldValue);  // precondition
  // Dereference safe as incValue does not retain ptr.
  e.setValue(
    t,
    m_violationId,
    std::abs(m_a * e.getValue(t, m_x) - m_b * e.getValue(t, m_y))
  );

#ifdef VERBOSE_TRACE
#include <iostream>
  std::cout << "Constraint Equal[" << m_id << "] notifying output changed to "
            << std::abs(m_a * e.getValue(t, m_x) - m_b * e.getValue(t, m_y)) << "\n";
#endif
}

void Equal::commit(const Timestamp& t, Engine& e) {
  // todo: do nodes validate themself or is it done by engine?
  // this->validate(t);
  e.commitIf(t, m_violationId);
}