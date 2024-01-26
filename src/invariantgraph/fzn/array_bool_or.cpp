

#include "invariantgraph/fzn/array_bool_or.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_or(FznInvariantGraph& invariantGraph,
                   const fznparser::BoolVarArray& boolVarArray,
                   const fznparser::BoolArg& reified) {
  std::vector<bool> fixedValues = getFixedValues(boolVarArray);

  if (reified.isFixed()) {
    if (reified.toParameter()) {
      if (fixedValues.size() == boolVarArray.size() &&
          std::all_of(fixedValues.begin(), fixedValues.end(),
                      [](bool b) { return !b; })) {
        throw FznArgumentException(
            "Constraint array_bool_or is unsatisfiable because the reified "
            "argument is fixed to true "
            "but the array contains only elements that are true.");
      }
    } else if (std::any_of(fixedValues.begin(), fixedValues.end(),
                           [](bool b) { return b; })) {
      throw FznArgumentException(
          "Constraint array_bool_or is unsatisfiable because the reified "
          "argument is fixed to false "
          "but the array contains an element that is true.");
    }
  }

  std::vector<VarNodeId> varNodeIds =
      invariantGraph.createVarNodes(boolVarArray, false);

  std::vector<VarNodeId> prunedVars;
  prunedVars.reserve(varNodeIds.size() - fixedValues.size());
  for (const VarNodeId& varNodeId : varNodeIds) {
    if (!invariantGraph.varNode(varNodeId).isFixed()) {
      prunedVars.emplace_back(varNodeId);
    }
  }

  if (prunedVars.empty()) {
    // TODO: throw exception?
    return true;
  }

  if (reified.isFixed()) {
    invariantGraph.addInvariantNode(
        std::make_unique<invariantgraph::ArrayBoolOrNode>(
            std::move(varNodeIds), reified.toParameter()));
    return true;
  }
  invariantGraph.addInvariantNode(
      std::make_unique<invariantgraph::ArrayBoolOrNode>(
          std::move(varNodeIds),
          invariantGraph.createVarNodeFromFzn(reified.var(), true)));
  return true;
}

bool array_bool_or(FznInvariantGraph& invariantGraph,
                   const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_bool_or") {
    return false;
  }
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::BoolVarArray, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true);
  return array_bool_or(
      invariantGraph,
      std::get<fznparser::BoolVarArray>(constraint.arguments().at(0)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn