#include "atlantis/invariantgraph/fzn/fzn_count_neq.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_count_neq(FznInvariantGraph& graph,
                   const std::shared_ptr<fznparser::IntVarArray>& inputs,
                   const fznparser::IntArg& needle,
                   const fznparser::IntArg& count) {
  const VarNodeId output = createCountNode(graph, inputs, needle);
  graph.addInvariantNode(std::make_shared<AllDifferentNode>(
      graph, output, graph.retrieveVarNode(count)));
  return true;
}

bool fzn_count_neq(FznInvariantGraph& graph,
                   const std::shared_ptr<fznparser::IntVarArray>& inputs,
                   const fznparser::IntArg& needle,
                   const fznparser::IntArg& count,
                   const fznparser::BoolArg& reified) {
  const VarNodeId output = createCountNode(graph, inputs, needle);
  graph.addInvariantNode(std::make_shared<AllDifferentNode>(
      graph, output, graph.retrieveVarNode(count),
      graph.retrieveVarNode(reified)));
  return true;
}

bool fzn_count_neq(FznInvariantGraph& graph,
                   const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_count_neq" &&
      constraint.identifier() != "fzn_count_neq_reif") {
    return false;
  }

  const bool isReified = constraintIdentifierIsReified(constraint);

  verifyNumArguments(constraint, isReified ? 4 : 3);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, true)
  if (!isReified) {
    return fzn_count_neq(
        graph,
        getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)),
        std::get<fznparser::IntArg>(constraint.arguments().at(1)),
        std::get<fznparser::IntArg>(constraint.arguments().at(2)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true)
  return fzn_count_neq(
      graph, getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)),
      std::get<fznparser::IntArg>(constraint.arguments().at(1)),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
