

#include "invariantgraph/fzn/array_bool_and.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_and(FznInvariantGraph& invariantGraph,
                    const fznparser::BoolVarArray boolVarArray,
                    const fznparser::BoolArg reified) {
  std::vector<bool> fixedValues = getFixedValues(boolVarArray);

  if (reified.isFixed()) {
    if (reified.toParameter()) {
      if (std::any_of(fixedValues.begin(), fixedValues.end(),
                      [](bool b) { return !b; })) {
        throw FznArgumentException(
            "Constraint array_bool_and is unsatisfiable because the reified "
            "argument is fixed to true "
            "but the array contains at least one element that is not true.");
      }
    } else if (fixedValues.size() == boolVarArray.size() &&
               std::all_of(fixedValues.begin(), fixedValues.end(),
                           [](bool b) { return b; })) {
      throw FznArgumentException(
          "Constraint array_bool_and is unsatisfiable because the reified "
          "argument is fixed to false "
          "but the array contains only elements that are true.");
    }
  }

  if (reified.isFixed()) {
    invariantGraph.addInvariantNode(
        std::make_unique<invariantgraph::ArrayBoolAndNode>(
            invariantGraph.createVarNodes(boolVarArray, false),
            reified.toParameter()));
    return true;
  }
  invariantGraph.addInvariantNode(
      std::make_unique<invariantgraph::ArrayBoolAndNode>(
          invariantGraph.createVarNodes(boolVarArray, false),
          invariantGraph.createVarNode(reified.var(), true)));
  return true;
}

bool array_bool_and(FznInvariantGraph& invariantGraph,
                    const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_bool_and") {
    return false;
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::BoolVarArray, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true);
  return array_bool_and(
      invariantGraph,
      std::get<fznparser::BoolVarArray>(constraint.arguments().at(0)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn