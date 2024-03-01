

#include "atlantis/invariantgraph/fzn/array_bool_xor.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_xor(FznInvariantGraph& invariantGraph,
                    const fznparser::BoolVarArray& vars,
                    const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    invariantGraph.addInvariantNode(
        std::make_unique<invariantgraph::ArrayBoolXorNode>(
            invariantGraph.retrieveVarNodes(vars), reified.toParameter()));
    return true;
  }
  invariantGraph.addInvariantNode(
      std::make_unique<invariantgraph::ArrayBoolXorNode>(
          invariantGraph.retrieveVarNodes(vars),
          invariantGraph.retrieveVarNode(reified.var())));
  return true;
}

bool array_bool_xor(FznInvariantGraph& invariantGraph,
                    const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_bool_xor") {
    return false;
  }
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::BoolVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true)
  return array_bool_xor(
      invariantGraph,
      std::get<fznparser::BoolVarArray>(constraint.arguments().at(0)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn
