#include "invariants/linear.hpp"
#include <vector>

Linear::Linear([[maybe_unused]] Engine& e, Id id, std::vector<Int>&& A,
               std::vector<std::shared_ptr<IntVar>>&& X)
    : Invariant(id), m_A(std::move(A)), m_X(std::move(X)) {}

void Linear::notifyIntChanged([[maybe_unused]] Engine& e, [[maybe_unused]] Id id,
                              [[maybe_unused]] Int oldValue, [[maybe_unused]] Int newValue,
                              [[maybe_unused]] Int data) {}