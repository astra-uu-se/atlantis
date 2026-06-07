#include "atlantis/invariantgraph/fzn/fzn_count_gt.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/countRelNode.hpp"

namespace atlantis::invariantgraph::fzn {

/*
 * Constrains count to be strictly greater than the number of occurrences of
 *needle in inputs.
 **/

bool fzn_count_gt(FznInvariantGraph& graph,
                  const std::shared_ptr<fznparser::IntVarArray>& inputs,
                  const fznparser::IntArg& needle,
                  const fznparser::IntArg& count) {
  graph.addInvariantNode(std::make_shared<CountRelNode>(
      graph, graph.retrieveVarNodes(inputs), graph.retrieveVarNode(needle),
      graph.retrieveVarNode(count), RelationType::REL_TYPE_GT));
  return true;
}

bool fzn_count_gt_reif(FznInvariantGraph& graph,
                       const std::shared_ptr<fznparser::IntVarArray>& inputs,
                       const fznparser::IntArg& needle,
                       const fznparser::IntArg& count,
                       const fznparser::BoolArg& reified) {
  graph.addInvariantNode(std::make_shared<CountRelNode>(
      graph, graph.retrieveVarNodes(inputs), graph.retrieveVarNode(needle),
      graph.retrieveVarNode(count), RelationType::REL_TYPE_GT,
      graph.retrieveVarNode(reified)));

  return true;
}

bool fzn_count_gt(FznInvariantGraph& graph,
                  const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_count_gt" &&
      constraint.identifier() != "fzn_count_gt_reif") {
    return false;
  }

  const bool isReified = constraintIdentifierIsReified(constraint);

  verifyNumArguments(constraint, isReified ? 4 : 3);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, true);
  if (!isReified) {
    return fzn_count_gt(
        graph,
        getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)),
        std::get<fznparser::IntArg>(constraint.arguments().at(1)),
        std::get<fznparser::IntArg>(constraint.arguments().at(2)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true);
  return fzn_count_gt_reif(
      graph, getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)),
      std::get<fznparser::IntArg>(constraint.arguments().at(1)),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(3)));
}

}  // namespace atlantis::invariantgraph::fzn
