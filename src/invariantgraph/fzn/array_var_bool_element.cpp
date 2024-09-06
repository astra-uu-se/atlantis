#include "atlantis/invariantgraph/fzn/array_var_bool_element.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_var_bool_element(
    FznInvariantGraph& graph, const fznparser::IntArg& index,
    const std::shared_ptr<fznparser::BoolVarArray>& inputs,
    const fznparser::BoolArg& output, Int offset) {
  graph.addInvariantNode(std::make_shared<ArrayVarElementNode>(
      graph, graph.retrieveVarNode(index), graph.retrieveVarNodes(inputs),
      graph.retrieveVarNode(output), offset));
  return true;
}

bool array_var_bool_element(FznInvariantGraph& graph,
                            const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_var_bool_element" &&
      constraint.identifier() != "array_var_bool_element_offset" &&
      constraint.identifier() != "array_var_bool_element_nonshifted") {
    return false;
  }

  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::BoolVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)

  const auto& index = std::get<fznparser::IntArg>(constraint.arguments().at(0));
  Int offset = 1;
  if (constraint.identifier() != "array_var_bool_element_nonshifted") {
    FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::IntArg, false)
    offset =
        std::get<fznparser::IntArg>(constraint.arguments().at(3)).toParameter();
  } else {
    // Compute offset if nonshifted variant:
    offset =
        index.isParameter() ? index.parameter() : index.var()->lowerBound();
  }

  return array_var_bool_element(
      graph, index,
      getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(1)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(2)), offset);
}

}  // namespace atlantis::invariantgraph::fzn
