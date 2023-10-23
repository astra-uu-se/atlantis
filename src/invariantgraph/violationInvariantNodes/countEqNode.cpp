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

std::unique_ptr<CountEqNode> CountEqNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntVarArray& xArg =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0));

  const fznparser::IntArg& yArg =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));

  VarNodeId yVarNode =
      yArg.isFixed() ? NULL_NODE_ID : invariantGraph.createVarNode(yArg.var());

  const Int yParameter = yArg.isFixed() ? yArg.toParameter() : -1;

  const fznparser::IntArg& cArg =
      std::get<fznparser::IntArg>(constraint.arguments().at(2));

  VarNodeId cVarNode =
      cArg.isFixed() ? NULL_NODE_ID : invariantGraph.createVarNode(cArg.var());

  const Int cParameter = cArg.isParameter() ? cArg.parameter() : -1;

  bool shouldHold = true;
  VarNodeId r = NULL_NODE_ID;

  if (constraint.arguments().size() == 4) {
    const fznparser::BoolArg& reified =
        std::get<fznparser::BoolArg>(constraint.arguments().back());

    if (reified.isFixed()) {
      shouldHold = reified.toParameter();
    } else {
      r = invariantGraph.createVarNode(reified.var());
    }
  }

  if (r != NULL_NODE_ID && !cArg.isParameter()) {
    throw std::runtime_error(
        "count_eq_reif(x, y, c, r): c must be a parameter, var given");
  }
  if (!shouldHold && !cArg.isParameter()) {
    throw std::runtime_error(
        "count_eq_reif(x, y, c, false): c must be a parameter, var given");
  }

  std::vector<VarNodeId> x = invariantGraph.createVarNodes(xArg);

  if (yArg.isFixed()) {
    if (cArg.isFixed()) {
      if (r != NULL_NODE_ID) {
        return std::make_unique<CountEqNode>(std::move(x), yParameter,
                                             cParameter, r);
      }
      return std::make_unique<CountEqNode>(std::move(x), yParameter, cParameter,
                                           shouldHold);
    }
    assert(r == NULL_NODE_ID);
    assert(shouldHold);
    return std::make_unique<CountEqNode>(std::move(x), yParameter, cVarNode,
                                         true);
  }
  if (cArg.isFixed()) {
    if (r != NULL_NODE_ID) {
      return std::make_unique<CountEqNode>(std::move(x), yVarNode, cParameter,
                                           r);
    }
    return std::make_unique<CountEqNode>(std::move(x), yVarNode, cParameter,
                                         shouldHold);
  }
  assert(r == NULL_NODE_ID);
  assert(shouldHold);
  return std::make_unique<CountEqNode>(std::move(x), yVarNode, cVarNode, true);
}

void CountEqNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                          propagation::Engine& engine) {
  if (!_cIsParameter) {
    if (invariantGraph.varId(cVarNode()) == propagation::NULL_ID) {
      assert(!isReified());
      assert(shouldHold());
      invariantGraph.varNode(cVarNode()).setVarId(engine.makeIntVar(0, 0, 0));
    }
  } else if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _intermediate = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(
          invariantGraph,
          engine.makeIntView<propagation::EqualConst>(engine, _intermediate, _cParameter));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph,
                        engine.makeIntView<propagation::NotEqualConst>(engine, _intermediate,
                                                          _cParameter));
    }
  }
}

void CountEqNode::registerNode(InvariantGraph& invariantGraph, propagation::Engine& engine) {
  std::vector<propagation::VarId> engineInputs;
  const size_t inputSize =
      staticInputVarNodeIds().size() - static_cast<size_t>(!_yIsParameter);

  engineInputs.resize(inputSize);

  for (size_t i = 0; i < inputSize; ++i) {
    engineInputs.at(i) = invariantGraph.varId(staticInputVarNodeIds().at(i));
  }

  if (!_yIsParameter) {
    assert(yVarNode() != NULL_NODE_ID);
    assert(invariantGraph.varId(yVarNode()) != propagation::NULL_ID);
    assert(_cIsParameter || (cVarNode() != NULL_NODE_ID));
    assert(_cIsParameter || (invariantGraph.varId(cVarNode()) != propagation::NULL_ID));
    assert(_cIsParameter == (_intermediate != propagation::NULL_ID));
    assert(_cIsParameter == (violationVarId(invariantGraph) != propagation::NULL_ID));
    engine.makeInvariant<propagation::Count>(
        engine,
        _cIsParameter ? _intermediate : invariantGraph.varId(cVarNode()),
        invariantGraph.varId(yVarNode()), engineInputs);
    return;
  }
  assert(_yIsParameter);
  if (!_cIsParameter) {
    assert(violationVarId(invariantGraph) == propagation::NULL_ID);
    assert(shouldHold());
    assert(!isReified());
    assert(cVarNode() != NULL_NODE_ID);
    assert(invariantGraph.varId(cVarNode()) != propagation::NULL_ID);
    engine.makeInvariant<propagation::CountConst>(engine, invariantGraph.varId(cVarNode()),
                                     _yParameter, engineInputs);
    return;
  }
  assert(_cIsParameter);
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  assert(_intermediate != propagation::NULL_ID);
  assert(cVarNode() == NULL_NODE_ID);
  engine.makeInvariant<propagation::CountConst>(engine, _intermediate, _yParameter,
                                   engineInputs);
}

}  // namespace invariantgraph