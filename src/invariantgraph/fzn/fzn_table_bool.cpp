#include "atlantis/invariantgraph/fzn/fzn_table_bool.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/tableInNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_table_bool(FznInvariantGraph& graph,
                    const std::shared_ptr<fznparser::BoolVarArray>& inputs,
                    std::vector<std::vector<bool>>&& table) {
  graph.addInvariantNode(std::make_shared<TableInNode>(
      graph, graph.retrieveVarNodes(inputs), std::move(table), true));
  return true;
}

bool fzn_table_bool(FznInvariantGraph& graph,
                    const std::shared_ptr<fznparser::BoolVarArray>& inputs,
                    std::vector<std::vector<bool>>&& table,
                    const fznparser::BoolArg& reified) {
  graph.addInvariantNode(std::make_shared<TableInNode>(
      graph, graph.retrieveVarNodes(inputs), std::move(table),
      graph.retrieveVarNode(reified)));
  return true;
}

bool fzn_table_bool(FznInvariantGraph& graph,
                    const fznparser::Constraint& constraint) {
  const bool isFlat = constraint.identifier() == "fzn_table_bool_flat" ||
                      constraint.identifier() == "fzn_table_bool_flat_reif";
  if (constraint.identifier() != "fzn_table_bool" &&
      constraint.identifier() != "fzn_table_bool_reif" &&
      !isFlat) {
    return false;
  }

  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 3 : 2);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::BoolVarArray, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::BoolVarArray, false)

  const auto& vars =
      getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(0));
  if (vars->size() == 0) {
    throw FznArgumentException(
        "Constraint fzn_table_bool the number of variables must be strictly "
        "positive.");
  }

  const std::vector<bool> flatTable =
      isFlat ? getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(1))
                     ->toParVector()
             : getArgArray<fznparser::BoolVarArray>(
                   getArgArray<fznparser::BoolVarArray>(
                       constraint.arguments().at(1)))
                   ->toParVector();

  if (flatTable.size() % vars->size() != 0) {
    throw FznArgumentException(
        "Constraint fzn_table_bool the number of variables must divide the "
        "number of elements in the array.");
  }
  std::vector<std::vector<bool>> table(
      flatTable.empty() ? 0 : flatTable.size() / vars->size(),
      std::vector<bool>(vars->size()));
  size_t i = 0;
  for (size_t r = 0; r < table.size(); ++r) {
    for (size_t c = 0; c < table[r].size(); ++c) {
      table[r][c] = flatTable[i++];
    }
  }

  if (!isReified) {
    return fzn_table_bool(graph, vars, std::move(table));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)
  return fzn_table_bool(
      graph, vars, std::move(table),
      std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
