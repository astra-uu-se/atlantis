#include "invariantgraph/constraints/countEqNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<CountEqNode> CountEqNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntVarArray& xArg =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0));

  const fznparser::IntArg& yArg =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));

  VariableNode* yVarNode =
      yArg.isParameter()
          ? nullptr
          : invariantGraph.addVariable(
                std::get<std::reference_wrapper<const fznparser::IntVar>>(yArg)
                    .get());

  const Int yParameter = yArg.isParameter() ? std::get<Int>(yArg) : -1;

  const fznparser::IntArg& cArg =
      std::get<fznparser::IntArg>(constraint.arguments().at(2));

  VariableNode* cVarNode =
      cArg.isParameter()
          ? nullptr
          : invariantGraph.addVariable(
                std::get<std::reference_wrapper<const fznparser::IntVar>>(cArg)
                    .get());

  const Int cParameter = cArg.isParameter() ? std::get<Int>(cArg) : -1;

  bool shouldHold = true;
  VariableNode* r = nullptr;

  if (constraint.arguments().size() == 4) {
    const fznparser::BoolArg& reified =
        std::get<fznparser::BoolArg>(constraint.arguments().at(3));

    if (std::holds_alternative<bool>(reified)) {
      shouldHold = std::get<bool>(reified);
    } else {
      r = invariantGraph.addVariable(
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(reified)
              .get());
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

  std::vector<VariableNode*> x = invariantGraph.addVariableArray(xArg);

  if (yArg.isParameter()) {
    if (cArg.isParameter()) {
      if (r != nullptr) {
        return std::make_unique<CountEqNode>(x, yParameter, cParameter, r);
      }
      return std::make_unique<CountEqNode>(x, yParameter, cParameter,
                                           shouldHold);
    }
    assert(r == nullptr);
    assert(shouldHold);
    return std::make_unique<CountEqNode>(x, yParameter, cVarNode, true);
  }
  if (cArg.isParameter()) {
    if (r != nullptr) {
      return std::make_unique<CountEqNode>(x, yVarNode, cParameter, r);
    }
    return std::make_unique<CountEqNode>(x, yVarNode, cParameter, shouldHold);
  }
  assert(r == nullptr);
  assert(shouldHold);
  return std::make_unique<CountEqNode>(x, yVarNode, cVarNode, true);
}

void CountEqNode::createDefinedVariables(Engine& engine) {
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

void CountEqNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> engineInputs;
  const size_t inputSize =
      staticInputs().size() - static_cast<size_t>(!_yIsParameter);

  engineInputs.resize(inputSize);

  for (size_t i = 0; i < inputSize; ++i) {
    engineInputs.at(i) = staticInputs().at(i)->varId();
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