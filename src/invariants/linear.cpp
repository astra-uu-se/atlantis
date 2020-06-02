#include "invariants/linear.hpp"
#include <vector>

Linear::Linear(Engine& e, Id id, std::vector<Int>&& A,
               std::vector<std::shared_ptr<IntVar>>&& X)
    : Invariant(id), m_A(std::move(A)), m_X(std::move(X)) {}

void Linear::notifyIntChanged(Engine& e, Id id, Int oldValue, Int newValue,
                              Int data) {

                              }