#include "invariantgraph/violationInvariantNodes/countLeqNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y,
                           Int yParameter, VarNodeId c, Int cParameter,
                           VarNodeId r)
    : ViolationInvariantNode(append(std::move(x), y, c), r),
      _yIsParameter(y == NULL_NODE_ID),
      _yParameter(yParameter),
      _cIsParameter(c == NULL_NODE_ID),
      _cParameter(cParameter) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y,
                           Int yParameter, VarNodeId c, Int cParameter,
                           bool shouldHold)
    : ViolationInvariantNode(append(std::move(x), y, c), shouldHold),
      _yIsParameter(y == NULL_NODE_ID),
      _yParameter(yParameter),
      _cIsParameter(c == NULL_NODE_ID),
      _cParameter(cParameter) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                           VarNodeId r)
    : CountLeqNode(std::move(x), y, 0, c, 0, r) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                           VarNodeId c, VarNodeId r)
    : CountLeqNode(std::move(x), NULL_NODE_ID, yParameter, c, 0, r) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y,
                           Int cParameter, VarNodeId r)
    : CountLeqNode(std::move(x), y, 0, NULL_NODE_ID, cParameter, r) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                           Int cParameter, VarNodeId r)
    : CountLeqNode(std::move(x), NULL_NODE_ID, yParameter, NULL_NODE_ID,
                   cParameter, r) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                           bool shouldHold)
    : CountLeqNode(std::move(x), y, 0, c, 0, shouldHold) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                           VarNodeId c, bool shouldHold)
    : CountLeqNode(std::move(x), NULL_NODE_ID, yParameter, c, 0, shouldHold) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, VarNodeId y,
                           Int cParameter, bool shouldHold)
    : CountLeqNode(std::move(x), y, 0, NULL_NODE_ID, cParameter, shouldHold) {}

CountLeqNode::CountLeqNode(std::vector<VarNodeId>&& x, Int yParameter,
                           Int cParameter, bool shouldHold)
    : CountLeqNode(std::move(x), NULL_NODE_ID, yParameter, NULL_NODE_ID,
                   cParameter, shouldHold) {}

void CountLeqNode::registerOutputVars(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _intermediate = solver.makeIntVar(0, 0, 0);
    if (!_cIsParameter) {
      registerViolation(invariantGraph, solver);
    } else {
      if (shouldHold()) {
        // Constrains c to be less than or equal to the number of occurrences of
        // y in x.
        // occurrences >= c
        setViolationVarId(invariantGraph,
                          solver.makeIntView<propagation::GreaterEqualConst>(
                              solver, _intermediate, _cParameter));
      } else {
        assert(!isReified());
        // occurrences < c
        // occurrences <= c - 1
        setViolationVarId(invariantGraph,
                          solver.makeIntView<propagation::LessEqualConst>(
                              solver, _intermediate, _cParameter - 1));
      }
    }
  }
}

void CountLeqNode::registerNode(InvariantGraph& invariantGraph,
                                propagation::SolverBase& solver) {
  std::vector<propagation::VarId> solverInputs;
  assert(staticInputVarNodeIds().size() >=
         static_cast<size_t>(!_yIsParameter) +
             static_cast<size_t>(!_cIsParameter));
  const size_t vectorSize = staticInputVarNodeIds().size() -
                            static_cast<size_t>(!_yIsParameter) -
                            static_cast<size_t>(!_cIsParameter);

  solverInputs.resize(vectorSize);

  for (size_t i = 0; i < vectorSize; ++i) {
    solverInputs.at(i) = invariantGraph.varId(staticInputVarNodeIds().at(i));
  }

  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  assert(_intermediate != propagation::NULL_ID);

  if (!_yIsParameter) {
    assert(yVarNode() != NULL_NODE_ID);
    assert(invariantGraph.varId(yVarNode()) != propagation::NULL_ID);
    solver.makeInvariant<propagation::Count>(
        solver, _intermediate, invariantGraph.varId(yVarNode()), solverInputs);
  } else {
    assert(yVarNode() == NULL_NODE_ID);
    solver.makeInvariant<propagation::CountConst>(solver, _intermediate,
                                                  _yParameter, solverInputs);
  }
  if (!_cIsParameter) {
    assert(cVarNode() != NULL_NODE_ID);
    assert(invariantGraph.varId(cVarNode()) != propagation::NULL_ID);
    if (shouldHold()) {
      solver.makeInvariant<propagation::LessEqual>(
          solver, violationVarId(invariantGraph),
          invariantGraph.varId(cVarNode()), _intermediate);
    } else {
      // c > count(x, y) -> count(x, y) < c
      solver.makeInvariant<propagation::LessThan>(
          solver, violationVarId(invariantGraph), _intermediate,
          invariantGraph.varId(cVarNode()));
    }
  }
}

[[nodiscard]] VarNodeId CountLeqNode::yVarNode() const {
  return _yIsParameter ? VarNodeId(NULL_NODE_ID)
                       : staticInputVarNodeIds().at(
                             staticInputVarNodeIds().size() -
                             (1 + static_cast<size_t>(!_cIsParameter)));
}

[[nodiscard]] VarNodeId CountLeqNode::cVarNode() const {
  return _cIsParameter ? VarNodeId(NULL_NODE_ID)
                       : staticInputVarNodeIds().back();
}

}  // namespace atlantis::invariantgraph