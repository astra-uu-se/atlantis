#include "atlantis/invariantgraph/fzn/fzn_all_equal_int.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/fzn/int_eq.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_all_equal_int(FznInvariantGraph& invariantGraph,
                       const std::shared_ptr<fznparser::IntVarArray>& inputs) {
  invariantGraph.addInvariantNode(std::make_unique<IntAllEqualNode>(
      invariantGraph.retrieveVarNodes(inputs)));
  return true;
}

bool fzn_all_equal_int(FznInvariantGraph& invariantGraph,
                       const std::shared_ptr<fznparser::IntVarArray>& inputs,
                       const fznparser::BoolArg& reified) {
  invariantGraph.addInvariantNode(std::make_unique<IntAllEqualNode>(
      invariantGraph.retrieveVarNodes(inputs),
      invariantGraph.retrieveVarNode(reified)));
  return true;
}

bool fzn_all_equal_int(FznInvariantGraph& invariantGraph,
                       const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_all_equal_int" &&
      constraint.identifier() != "fzn_all_equal_int_reif") {
    return false;
  }

  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 2 : 1);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)

  if (!isReified) {
    return fzn_all_equal_int(invariantGraph,
                             std::get<std::shared_ptr<fznparser::IntVarArray>>(
                                 constraint.arguments().at(0)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true)
  return fzn_all_equal_int(
      invariantGraph,
      std::get<std::shared_ptr<fznparser::IntVarArray>>(
          constraint.arguments().at(0)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn
