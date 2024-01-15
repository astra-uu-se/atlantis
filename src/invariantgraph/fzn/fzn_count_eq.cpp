#include "invariantgraph/fzn/fzn_count_eq.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "invariantgraph/fzn/fzn_count_neq.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_count_eq(FznInvariantGraph& invariantGraph,
                  const fznparser::IntVarArray& inputs,
                  const fznparser::IntArg& needle,
                  const fznparser::IntArg& count) {
  createCountNode(invariantGraph, inputs, needle, count);
  return true;
}

bool fzn_count_eq(FznInvariantGraph& invariantGraph,
                  const fznparser::IntVarArray& inputs,
                  const fznparser::IntArg& needle,
                  const fznparser::IntArg& count,
                  const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return fzn_count_eq(invariantGraph, inputs, needle, count);
    }
    return fzn_count_neq(invariantGraph, inputs, needle, count);
  }
  const VarNodeId output = createCountNode(invariantGraph, inputs, needle);
  return int_eq(invariantGraph, output,
                invariantGraph.createVarNodeFromFzn(count, false), reified);
}

bool fzn_count_eq(FznInvariantGraph& invariantGraph,
                  const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_count_eq" &&
      constraint.identifier() != "fzn_count_eq_reif") {
    return false;
  }

  const bool isReified = constraintIdentifierIsReified(constraint);

  verifyNumArguments(constraint, isReified ? 4 : 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, true);
  if (!isReified) {
    return fzn_count_eq(
        invariantGraph,
        std::get<fznparser::IntVarArray>(constraint.arguments().at(0)),
        std::get<fznparser::IntArg>(constraint.arguments().at(1)),
        std::get<fznparser::IntArg>(constraint.arguments().at(2)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true);
  return fzn_count_eq(
      invariantGraph,
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0)),
      std::get<fznparser::IntArg>(constraint.arguments().at(1)),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn