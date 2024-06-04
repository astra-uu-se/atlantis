

#include "atlantis/invariantgraph/fzn/array_bool_or.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_or(FznInvariantGraph& invariantGraph,
                   const std::shared_ptr<fznparser::BoolVarArray>& boolVarArray,
                   const fznparser::BoolArg& reified) {
  std::vector<bool> fixedValues = getFixedValues(boolVarArray);

  invariantGraph.addInvariantNode(
      std::make_unique<invariantgraph::ArrayBoolOrNode>(
          invariantGraph.retrieveVarNodes(boolVarArray),
          invariantGraph.retrieveVarNode(reified)));
  return true;
}

bool array_bool_or(FznInvariantGraph& invariantGraph,
                   const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_bool_or") {
    return false;
  }
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::BoolVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true)
  return array_bool_or(
      invariantGraph,
      std::get<std::shared_ptr<fznparser::BoolVarArray>>(
          constraint.arguments().at(0)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn
