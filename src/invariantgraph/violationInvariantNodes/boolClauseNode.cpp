#include "invariantgraph/violationInvariantNodes/boolClauseNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

BoolClauseNode::BoolClauseNode(std::vector<VarNodeId>&& as,
                               std::vector<VarNodeId>&& bs, VarNodeId r)
    : ViolationInvariantNode(std::move(concat(as, bs)), r),
      _as(std::move(as)),
      _bs(std::move(bs)) {}
BoolClauseNode::BoolClauseNode(std::vector<VarNodeId>&& as,
                               std::vector<VarNodeId>&& bs, bool shouldHold)
    : ViolationInvariantNode(std::move(concat(as, bs)), shouldHold),
      _as(std::move(as)),
      _bs(std::move(bs)) {}

std::unique_ptr<BoolClauseNode> BoolClauseNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 &&
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
  if (constraint.arguments().size() == 3 &&
      !std::holds_alternative<fznparser::BoolArg>(
          constraint.arguments().back())) {
    throw std::runtime_error(
        "boolClause constraint optional third argument must be a bool "
        "var");
  }
  std::vector<VarNodeId> as = invariantGraph.createVarNodes(
      get<fznparser::BoolVarArray>(constraint.arguments().at(0)));

  std::vector<VarNodeId> bs = invariantGraph.createVarNodes(
      get<fznparser::BoolVarArray>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<BoolClauseNode>(std::move(as), std::move(bs), true);
  }

  const fznparser::BoolArg& reified =
      get<fznparser::BoolArg>(constraint.arguments().back());
  if (reified.isFixed()) {
    return std::make_unique<invariantgraph::BoolClauseNode>(
        std::move(as), std::move(bs), reified.toParameter());
  }
  return std::make_unique<invariantgraph::BoolClauseNode>(
      std::move(as), std::move(bs),
      invariantGraph.createVarNode(reified.var()));
}

void BoolClauseNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                             Engine& engine) {
  if (violationVarId(invariantGraph) == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph, engine.makeIntView<EqualConst>(
                                            engine, _sumVarId,
                                            static_cast<Int>(_as.size()) +
                                                static_cast<Int>(_bs.size())));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph, engine.makeIntView<NotEqualConst>(
                                            engine, _sumVarId,
                                            static_cast<Int>(_as.size()) +
                                                static_cast<Int>(_bs.size())));
    }
  }
}

void BoolClauseNode::registerNode(InvariantGraph& invariantGraph,
                                  Engine& engine) {
  std::vector<VarId> engineVariables;
  engineVariables.reserve(_as.size() + _bs.size());
  std::transform(_as.begin(), _as.end(), std::back_inserter(engineVariables),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  std::transform(_bs.begin(), _bs.end(), std::back_inserter(engineVariables),
                 [&](const auto& id) {
                   return engine.makeIntView<NotEqualConst>(
                       engine, invariantGraph.varId(id), 0);
                 });

  assert(_sumVarId != NULL_ID);
  assert(violationVarId(invariantGraph) != NULL_ID);
  engine.makeInvariant<BoolLinear>(engine, _sumVarId, engineVariables);
}

}  // namespace invariantgraph