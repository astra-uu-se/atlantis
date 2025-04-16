#include "atlantis/invariantgraph/fzn/array_bool_element2d.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_element2d(FznInvariantGraph& graph,
                          const fznparser::IntArg& rowIndex,
                          const fznparser::IntArg& colIndex,
                          std::vector<bool>&& parVector,
                          const fznparser::BoolArg& output, Int numRows,
                          Int rowOffset, Int colOffset) {
  if (numRows <= 0 || parVector.size() % numRows != 0) {
    throw FznArgumentException(
        "Constraint array_bool_element2d the number of rows must be strictly "
        "positive and a divide the number of elements in the array.");
  }

  if (rowOffset > (rowIndex.isParameter() ? rowIndex.toParameter()
                                          : rowIndex.var()->lowerBound())) {
    throw FznArgumentException(
        "Constraint array_bool_element2d the first offset must be smaller than "
        "the lower bound of the first index var.");
  }

  if (colOffset > (colIndex.isParameter() ? colIndex.toParameter()
                                          : colIndex.var()->lowerBound())) {
    throw FznArgumentException(
        "Constraint array_bool_element2d the second offset must be smaller "
        "than the lower bound of the second index var.");
  }

  const size_t numCols = parVector.size() / static_cast<size_t>(numRows);

  std::vector<std::vector<bool>> parMatrix(numRows, std::vector<bool>(numCols));
  for (Int i = 0; i < numRows; ++i) {
    for (size_t j = 0; j < numCols; ++j) {
      parMatrix.at(i).at(j) = parVector.at(i * numCols + j);
    }
  }

  graph.addInvariantNode(std::make_shared<ArrayElement2dNode>(
      graph, graph.retrieveVarNode(rowIndex), graph.retrieveVarNode(colIndex),
      std::move(parMatrix), graph.retrieveVarNode(output), rowOffset,
      colOffset));
  return true;
}

bool array_bool_element2d(FznInvariantGraph& graph,
                          const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_bool_element2d" &&
      constraint.identifier() != "array_bool_element2d_nonshifted_flat") {
    return false;
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 2, fznparser::BoolVarArray, false)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 4, fznparser::IntArg, false)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 5, fznparser::IntArg, false)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 6, fznparser::IntArg, false)

  return array_bool_element2d(
      graph, std::get<fznparser::IntArg>(constraint.arguments().at(0)),
      std::get<fznparser::IntArg>(constraint.arguments().at(1)),
      getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(2))
          ->toParVector(),
      std::get<fznparser::BoolArg>(constraint.arguments().at(3)),
      std::get<fznparser::IntArg>(constraint.arguments().at(4)).toParameter(),
      std::get<fznparser::IntArg>(constraint.arguments().at(5)).toParameter(),
      std::get<fznparser::IntArg>(constraint.arguments().at(6)).toParameter());
}

}  // namespace atlantis::invariantgraph::fzn
