#include "invariantgraph/violationInvariantNodes/countEqNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<CountEqNode> CountEqNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntVarArray& xArg =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0));

  const fznparser::IntArg& yArg =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));

  VarNodeId yVarNode =
      yArg.isFixed() ? nullptr : invariantGraph.createVarNode(yArg.var());

  const Int yParameter = yArg.isFixed() ? yArg.toParameter() : -1;

  const fznparser::IntArg& cArg =
      std::get<fznparser::IntArg>(constraint.arguments().at(2));

  VarNodeId cVarNode =
      cArg.isFixed() ? nullptr : invariantGraph.createVarNode(cArg.var());

  const Int cParameter = cArg.isParameter() ? cArg.parameter() : -1;

  bool shouldHold = true;
  VarNodeId r = nullptr;

  if (constraint.arguments().size() == 4) {
    const fznparser::BoolArg& reified =
        std::get<fznparser::BoolArg>(constraint.arguments().back());

    if (reified.isFixed()) {
      shouldHold = reified.toParameter();
    } else {
      r = invariantGraph.createVarNode(reified.var());
    }
  }

  if (r != nullptr && !cArg.isParameter()) {
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
      if (r != nullptr) {
        return std::make_unique<CountEqNode>(std::move(x), yParameter,
                                             cParameter, r);
      }
      return std::make_unique<CountEqNode>(std::move(x), yParameter, cParameter,
                                           shouldHold);
    }
    assert(r == nullptr);
    assert(shouldHold);
    return std::make_unique<CountEqNode>(std::move(x), yParameter, cVarNode,
                                         true);
  }
  if (cArg.isFixed()) {
    if (r != nullptr) {
      return std::make_unique<CountEqNode>(std::move(x), yVarNode, cParameter,
                                           r);
    }
    return std::make_unique<CountEqNode>(std::move(x), yVarNode, cParameter,
                                         shouldHold);
  }
  assert(r == nullptr);
  assert(shouldHold);
  return std::make_unique<CountEqNode>(std::move(x), yVarNode, cVarNode, true);
}

void CountEqNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                          Engine& engine) {
  if (!_cIsParameter) {
    if (cVarNode()->varId() == NULL_ID) {
      assert(!isReified());
      assert(shouldHold());
      cVarNode()->setVarId(engine.makeIntVar(0, 0, 0));
    }
  } else if (violationVarId() == NULL_ID) {
    _intermediate = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(
          engine.makeIntView<EqualConst>(engine, _intermediate, _cParameter));
    } else {
      assert(!isReified());
      setViolationVarId(engine.makeIntView<NotEqualConst>(engine, _intermediate,
                                                          _cParameter));
    }
  }
}

void CountEqNode::registerNode(InvariantGraph& invariantGraph, Engine& engine) {
  std::vector<VarId> engineInputs;
  const size_t inputSize =
      staticInputVarNodeIds().size() - static_cast<size_t>(!_yIsParameter);

  engineInputs.resize(inputSize);

  for (size_t i = 0; i < inputSize; ++i) {
    engineInputs.at(i) = staticInputVarNodeIds().at(i)->varId();
  }

  if (!_yIsParameter) {
    assert(yVarNode() != nullptr);
    assert(yVarNode()->varId() != NULL_ID);
    assert(_cIsParameter || (cVarNode() != nullptr));
    assert(_cIsParameter || (cVarNode()->varId() != NULL_ID));
    assert(_cIsParameter == (_intermediate != NULL_ID));
    assert(_cIsParameter == (violationVarId() != NULL_ID));
    engine.makeInvariant<Count>(
        engine, _cIsParameter ? _intermediate : cVarNode()->varId(),
        yVarNode()->varId(), engineInputs);
    return;
  }
  assert(_yIsParameter);
  if (!_cIsParameter) {
    assert(violationVarId() == NULL_ID);
    assert(shouldHold());
    assert(!isReified());
    assert(cVarNode() != nullptr);
    assert(cVarNode()->varId() != NULL_ID);
    engine.makeInvariant<CountConst>(engine, cVarNode()->varId(), _yParameter,
                                     engineInputs);
    return;
  }
  assert(_cIsParameter);
  assert(violationVarId() != NULL_ID);
  assert(_intermediate != NULL_ID);
  assert(cVarNode() == nullptr);
  engine.makeInvariant<CountConst>(engine, _intermediate, _yParameter,
                                   engineInputs);
}

}  // namespace invariantgraph