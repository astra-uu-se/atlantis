#include "invariantgraph/violationInvariantNodes/countGtNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

CountGtNode::CountGtNode(std::vector<VarNodeId>&& x, VarNodeId y,
                         Int yParameter, VarNodeId c, Int cParameter,
                         VarNodeId r)
    : ViolationInvariantNode(append(std::move(x), y, c), r),
      _yIsParameter(y == NULL_NODE_ID),
      _yParameter(yParameter),
      _cIsParameter(c == NULL_NODE_ID),
      _cParameter(cParameter) {}

CountGtNode::CountGtNode(std::vector<VarNodeId>&& x, VarNodeId y,
                         Int yParameter, VarNodeId c, Int cParameter,
                         bool shouldHold)
    : ViolationInvariantNode(append(std::move(x), y, c), shouldHold),
      _yIsParameter(y == NULL_NODE_ID),
      _yParameter(yParameter),
      _cIsParameter(c == NULL_NODE_ID),
      _cParameter(cParameter) {}

CountGtNode::CountGtNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                         VarNodeId r)
    : CountGtNode(std::move(x), y, 0, c, 0, r) {}

CountGtNode::CountGtNode(std::vector<VarNodeId>&& x, Int yParameter,
                         VarNodeId c, VarNodeId r)
    : CountGtNode(std::move(x), NULL_NODE_ID, yParameter, c, 0, r) {}

CountGtNode::CountGtNode(std::vector<VarNodeId>&& x, VarNodeId y,
                         Int cParameter, VarNodeId r)
    : CountGtNode(std::move(x), y, 0, NULL_NODE_ID, cParameter, r) {}

CountGtNode::CountGtNode(std::vector<VarNodeId>&& x, Int yParameter,
                         Int cParameter, VarNodeId r)
    : CountGtNode(std::move(x), NULL_NODE_ID, yParameter, NULL_NODE_ID,
                  cParameter, r) {}

CountGtNode::CountGtNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                         bool shouldHold)
    : CountGtNode(std::move(x), y, 0, c, 0, shouldHold) {}

CountGtNode::CountGtNode(std::vector<VarNodeId>&& x, Int yParameter,
                         VarNodeId c, bool shouldHold)
    : CountGtNode(std::move(x), NULL_NODE_ID, yParameter, c, 0, shouldHold) {}

CountGtNode::CountGtNode(std::vector<VarNodeId>&& x, VarNodeId y,
                         Int cParameter, bool shouldHold)
    : CountGtNode(std::move(x), y, 0, NULL_NODE_ID, cParameter, shouldHold) {}

CountGtNode::CountGtNode(std::vector<VarNodeId>&& x, Int yParameter,
                         Int cParameter, bool shouldHold)
    : CountGtNode(std::move(x), NULL_NODE_ID, yParameter, NULL_NODE_ID,
                  cParameter, shouldHold) {}

void CountGtNode::registerOutputVars(InvariantGraph& invariantGraph,
                                     propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _intermediate = solver.makeIntVar(0, 0, 0);
    if (!_cIsParameter) {
      registerViolation(invariantGraph, solver);
    } else {
      if (shouldHold()) {
        // Constrains c to be strictly greater than the number of occurrences of
        // y in x.
        // occurrences < c
        // occurrences <= c - 1
        setViolationVarId(invariantGraph,
                          solver.makeIntView<propagation::LessEqualConst>(
                              solver, _intermediate, _cParameter - 1));
      } else {
        // occurrences >= c
        assert(!isReified());
        setViolationVarId(invariantGraph,
                          solver.makeIntView<propagation::GreaterEqualConst>(
                              solver, _intermediate, _cParameter));
      }
    }
  }
}

void CountGtNode::registerNode(InvariantGraph& invariantGraph,
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
      // c > count(x, y) -> count(x, y) < c
      solver.makeInvariant<propagation::LessThan>(
          solver, violationVarId(invariantGraph), _intermediate,
          invariantGraph.varId(cVarNode()));
    } else {
      solver.makeInvariant<propagation::LessEqual>(
          solver, violationVarId(invariantGraph),
          invariantGraph.varId(cVarNode()), _intermediate);
    }
  }
}

VarNodeId CountGtNode::yVarNode() const {
  return _yIsParameter ? VarNodeId(NULL_NODE_ID)
                       : staticInputVarNodeIds().at(
                             staticInputVarNodeIds().size() -
                             (1 + static_cast<size_t>(!_cIsParameter)));
}

VarNodeId CountGtNode::cVarNode() const {
  return _cIsParameter ? VarNodeId(NULL_NODE_ID)
                       : staticInputVarNodeIds().back();
}

}  // namespace atlantis::invariantgraph