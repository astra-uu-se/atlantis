#include "invariantgraph/constraints/countEqNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/count.hpp"
#include "invariants/countConst.hpp"
#include "views/equalView.hpp"
#include "views/notEqualView.hpp"

std::unique_ptr<invariantgraph::CountEqNode>
invariantgraph::CountEqNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      (constraint.name == "fzn_count_eq" && constraint.arguments.size() == 3) ||
      (constraint.name == "fzn_count_eq_reif" &&
       constraint.arguments.size() == 4));

  auto x = mappedVariableVector(model, constraint.arguments[0], variableMap);

  bool yIsParameter = std::holds_alternative<Int>(constraint.arguments[1]);
  VariableNode* yVarNode =
      yIsParameter ? nullptr
                   : mappedVariable(constraint.arguments[1], variableMap);
  const Int yParameter =
      yIsParameter ? std::get<Int>(constraint.arguments[1]) : -1;

  bool cIsParameter = std::holds_alternative<Int>(constraint.arguments[2]);
  VariableNode* cVarNode =
      cIsParameter ? nullptr
                   : mappedVariable(constraint.arguments[2], variableMap);
  const Int cParameter =
      cIsParameter ? std::get<Int>(constraint.arguments[2]) : -1;

  bool shouldHold = true;
  VariableNode* r = nullptr;

  if (constraint.arguments.size() >= 4) {
    if (std::holds_alternative<bool>(constraint.arguments[3])) {
      shouldHold = std::get<bool>(constraint.arguments[3]);
    } else {
      r = mappedVariable(constraint.arguments[3], variableMap);
    }
  }

  if (r != nullptr && !cIsParameter) {
    throw std::runtime_error(
        "count_eq_reif(x, y, c, r): c must be a parameter, var given");
  }
  if (!shouldHold && !cIsParameter) {
    throw std::runtime_error(
        "count_eq_reif(x, y, c, false): c must be a parameter, var given");
  }

  if (yIsParameter) {
    if (cIsParameter) {
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
  if (cIsParameter) {
    if (r != nullptr) {
      return std::make_unique<CountEqNode>(x, yVarNode, cParameter, r);
    }
    return std::make_unique<CountEqNode>(x, yVarNode, cParameter, shouldHold);
  }
  assert(r == nullptr);
  assert(shouldHold);
  return std::make_unique<CountEqNode>(x, yVarNode, cVarNode, true);
}

void invariantgraph::CountEqNode::createDefinedVariables(Engine& engine) {
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
          engine.makeIntView<EqualView>(_intermediate, _cParameter));
    } else {
      assert(!isReified());
      setViolationVarId(
          engine.makeIntView<NotEqualView>(_intermediate, _cParameter));
    }
  }
}

void invariantgraph::CountEqNode::registerWithEngine(Engine& engine) {
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
        yVarNode()->varId(), engineInputs,
        _cIsParameter ? _intermediate : cVarNode()->varId());
    return;
  }
  assert(_yIsParameter);
  if (!_cIsParameter) {
    assert(violationVarId() == NULL_ID);
    assert(shouldHold());
    assert(!isReified());
    assert(cVarNode() != nullptr);
    assert(cVarNode()->varId() != NULL_ID);
    engine.makeInvariant<CountConst>(_yParameter, engineInputs,
                                     cVarNode()->varId());
    return;
  }
  assert(_cIsParameter);
  assert(violationVarId() != NULL_ID);
  assert(_intermediate != NULL_ID);
  assert(cVarNode() == nullptr);
  engine.makeInvariant<CountConst>(_yParameter, engineInputs, _intermediate);
}
