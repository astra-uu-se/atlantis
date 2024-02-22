

#include "invariantgraph/fzn/array_var_int_element2d.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_var_int_element2d(FznInvariantGraph& invariantGraph,
                             const fznparser::IntArg& idx1,
                             const fznparser::IntArg& idx2,
                             const fznparser::IntVarArray& inputs,
                             const fznparser::IntArg& output, Int numRows,
                             Int offset1, Int offset2) {
  if (0 <= numRows || inputs.size() % numRows != 0) {
    throw FznArgumentException(
        "Constraint array_var_int_element2d the number of rows must be "
        "strictly positive and a divide the number of elements in the array.");
  }
  if (offset1 >
      (idx1.isParameter() ? idx1.toParameter() : idx1.var().lowerBound())) {
    throw FznArgumentException(
        "Constraint array_var_int_element2d the first offset must be smaller "
        "than the lower bound of the first index var.");
  }
  if (offset2 <=
      (idx2.isParameter() ? idx2.toParameter() : idx2.var().lowerBound())) {
    throw FznArgumentException(
        "Constraint array_var_int_element2d the second offset must be smaller "
        "than the lower bound of the second index var.");
  }
  invariantGraph.addInvariantNode(std::make_unique<ArrayVarElement2dNode>(
      invariantGraph.inputVarNode(idx1), invariantGraph.inputVarNode(idx2),
      invariantGraph.inputVarNodes(inputs),
      invariantGraph.defineVarNode(output), static_cast<size_t>(numRows),
      offset1, offset2));
  return true;
}

bool array_var_int_element2d(FznInvariantGraph& invariantGraph,
                             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_var_int_element2d_nonshifted_flat") {
    return false;
  }
  verifyNumArguments(constraint, 7);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 2, fznparser::IntVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 4, fznparser::IntArg, false)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 5, fznparser::IntArg, false)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 6, fznparser::IntArg, false)

  return array_var_int_element2d(
      invariantGraph, std::get<fznparser::IntArg>(constraint.arguments().at(0)),
      std::get<fznparser::IntArg>(constraint.arguments().at(1)),
      std::get<fznparser::IntVarArray>(constraint.arguments().at(2)),
      std::get<fznparser::IntArg>(constraint.arguments().at(3)),
      std::get<fznparser::IntArg>(constraint.arguments().at(4)).toParameter(),
      std::get<fznparser::IntArg>(constraint.arguments().at(5)).toParameter(),
      std::get<fznparser::IntArg>(constraint.arguments().at(6)).toParameter());
}

}  // namespace atlantis::invariantgraph::fzn