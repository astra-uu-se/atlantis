#include "invariantgraph/violationInvariantNodes/countEqNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

CountEqNode::CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y,
                         Int yParameter, VarNodeId c, Int cParameter,
                         VarNodeId r)
    : ViolationInvariantNode(c == NULL_NODE_ID ? std::vector<VarNodeId>{}
                                               : std::vector<VarNodeId>{c},
                             append(std::move(x), y), r),
      _yIsParameter(y == NULL_NODE_ID),
      _yParameter(yParameter),
      _cIsParameter(c == NULL_NODE_ID),
      _cParameter(cParameter) {}

CountEqNode::CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y,
                         Int yParameter, VarNodeId c, Int cParameter,
                         bool shouldHold)
    : ViolationInvariantNode(c == NULL_NODE_ID ? std::vector<VarNodeId>{}
                                               : std::vector<VarNodeId>{c},
                             append(std::move(x), y), shouldHold),
      _yIsParameter(y == NULL_NODE_ID),
      _yParameter(yParameter),
      _cIsParameter(c == NULL_NODE_ID),
      _cParameter(cParameter) {}

CountEqNode::CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y,
                         Int cParameter, VarNodeId r)
    : CountEqNode(std::move(x), y, 0, NULL_NODE_ID, cParameter, r) {}

CountEqNode::CountEqNode(std::vector<VarNodeId>&& x, Int yParameter,
                         Int cParameter, VarNodeId r)
    : CountEqNode(std::move(x), NULL_NODE_ID, yParameter, NULL_NODE_ID,
                  cParameter, r) {}

CountEqNode::CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y, VarNodeId c,
                         bool shouldHold)
    : CountEqNode(std::move(x), y, 0, c, 0, shouldHold) {}

CountEqNode::CountEqNode(std::vector<VarNodeId>&& x, Int yParameter,
                         VarNodeId c, bool shouldHold)
    : CountEqNode(std::move(x), NULL_NODE_ID, yParameter, c, 0, shouldHold) {}

CountEqNode::CountEqNode(std::vector<VarNodeId>&& x, VarNodeId y,
                         Int cParameter, bool shouldHold)
    : CountEqNode(std::move(x), y, 0, NULL_NODE_ID, cParameter, shouldHold) {}

CountEqNode::CountEqNode(std::vector<VarNodeId>&& x, Int yParameter,
                         Int cParameter, bool shouldHold)
    : CountEqNode(std::move(x), NULL_NODE_ID, yParameter, NULL_NODE_ID,
                  cParameter, shouldHold) {}

void CountEqNode::registerOutputVars(InvariantGraph& invariantGraph,
                                     propagation::SolverBase& solver) {
  if (!_cIsParameter) {
    if (invariantGraph.varId(cVarNode()) == propagation::NULL_ID) {
      assert(!isReified());
      assert(shouldHold());
      invariantGraph.varNode(cVarNode()).setVarId(solver.makeIntVar(0, 0, 0));
    }
  } else if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _intermediate = solver.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::EqualConst>(
                            solver, _intermediate, _cParameter));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _intermediate, _cParameter));
    }
  }
}

void CountEqNode::registerNode(InvariantGraph& invariantGraph,
                               propagation::SolverBase& solver) {
  std::vector<propagation::VarId> solverInputs;
  const size_t inputSize =
      staticInputVarNodeIds().size() - static_cast<size_t>(!_yIsParameter);

  solverInputs.resize(inputSize);

  for (size_t i = 0; i < inputSize; ++i) {
    solverInputs.at(i) = invariantGraph.varId(staticInputVarNodeIds().at(i));
  }

  if (!_yIsParameter) {
    assert(yVarNode() != NULL_NODE_ID);
    assert(invariantGraph.varId(yVarNode()) != propagation::NULL_ID);
    assert(_cIsParameter || (cVarNode() != NULL_NODE_ID));
    assert(_cIsParameter ||
           (invariantGraph.varId(cVarNode()) != propagation::NULL_ID));
    assert(_cIsParameter == (_intermediate != propagation::NULL_ID));
    assert(_cIsParameter ==
           (violationVarId(invariantGraph) != propagation::NULL_ID));
    solver.makeInvariant<propagation::Count>(
        solver,
        _cIsParameter ? _intermediate : invariantGraph.varId(cVarNode()),
        invariantGraph.varId(yVarNode()), solverInputs);
    return;
  }
  assert(_yIsParameter);
  if (!_cIsParameter) {
    assert(violationVarId(invariantGraph) == propagation::NULL_ID);
    assert(shouldHold());
    assert(!isReified());
    assert(cVarNode() != NULL_NODE_ID);
    assert(invariantGraph.varId(cVarNode()) != propagation::NULL_ID);
    solver.makeInvariant<propagation::CountConst>(
        solver, invariantGraph.varId(cVarNode()), _yParameter, solverInputs);
    return;
  }
  assert(_cIsParameter);
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  assert(_intermediate != propagation::NULL_ID);
  assert(cVarNode() == NULL_NODE_ID);
  solver.makeInvariant<propagation::CountConst>(solver, _intermediate,
                                                _yParameter, solverInputs);
}

}  // namespace atlantis::invariantgraph