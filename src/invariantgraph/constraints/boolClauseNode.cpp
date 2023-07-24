#include "invariantgraph/constraints/boolClauseNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<BoolClauseNode> BoolClauseNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 ||
      constraint.arguments().size() != 3) {
    throw std::runtime_error("boolClause constraint takes two arguments");
  }
  if (!std::holds_alternative<fznparser::BoolVarArray>(
          constraint.arguments().at(0))) {
    throw std::runtime_error(
        "boolClause constraint first argument must be a bool var array");
  }
  if (!std::holds_alternative<fznparser::BoolVarArray>(
          constraint.arguments().at(1))) {
    throw std::runtime_error(
        "boolClause constraint second argument must be a bool var array");
  }
  if (!std::holds_alternative<fznparser::BoolArg>(
          constraint.arguments().back())) {
    throw std::runtime_error(
        "boolClause constraint optional third argument must be a bool "
        "var");
  }
  std::vector<VariableNode*> as = invariantGraph.addVariableArray(
      get<fznparser::BoolVarArray>(constraint.arguments().at(0)));

  std::vector<VariableNode*> bs = invariantGraph.addVariableArray(
      get<fznparser::BoolVarArray>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<BoolClauseNode>(std::move(as), std::move(bs), true);
  }

  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());

  if (std::holds_alternative<bool>(reified)) {
    return std::make_unique<BoolClauseNode>(std::move(as), std::move(bs),
                                            std::get<bool>(reified));
  }
  return std::make_unique<BoolClauseNode>(
      std::move(as), std::move(bs),
      invariantGraph.addVariable(
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(reified)
              .get()));
}

void BoolClauseNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(engine.makeIntView<EqualConst>(
          engine, _sumVarId,
          static_cast<Int>(_as.size()) + static_cast<Int>(_bs.size())));
    } else {
      assert(!isReified());
      setViolationVarId(engine.makeIntView<NotEqualConst>(
          engine, _sumVarId,
          static_cast<Int>(_as.size()) + static_cast<Int>(_bs.size())));
    }
  }
}

void BoolClauseNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> engineVariables;
  engineVariables.reserve(_as.size() + _bs.size());
  std::transform(_as.begin(), _as.end(), std::back_inserter(engineVariables),
                 [&](const auto& var) { return var->varId(); });

  std::transform(_bs.begin(), _bs.end(), std::back_inserter(engineVariables),
                 [&](const auto& var) {
                   return engine.makeIntView<NotEqualConst>(engine,
                                                            var->varId(), 0);
                 });

  assert(_sumVarId != NULL_ID);
  assert(violationVarId() != NULL_ID);
  engine.makeInvariant<BoolLinear>(engine, _sumVarId, engineVariables);
}

}  // namespace invariantgraph