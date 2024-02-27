

#include "invariantgraph/fzn/fzn_count_geq.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "invariantgraph/fzn/fzn_count_lt.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_count_geq(FznInvariantGraph& invariantGraph,
                   const fznparser::IntVarArray& inputs,
                   const fznparser::IntArg& needle,
                   const fznparser::IntArg& count) {
  VarNodeId output = createCountNode(invariantGraph, inputs, needle);
  return int_le(invariantGraph, invariantGraph.retrieveVarNode(count), output);
}

bool fzn_count_geq(FznInvariantGraph& invariantGraph,
                   const fznparser::IntVarArray& inputs,
                   const fznparser::IntArg& needle,
                   const fznparser::IntArg& count,
                   const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return fzn_count_geq(invariantGraph, inputs, needle, count);
    }
    return fzn_count_lt(invariantGraph, inputs, needle, count);
  }
  VarNodeId output = createCountNode(invariantGraph, inputs, needle);
  return int_le(invariantGraph, invariantGraph.retrieveVarNode(count), output,
                reified);
}

bool fzn_count_geq(FznInvariantGraph& invariantGraph,
                   const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_count_geq" &&
      constraint.identifier() != "fzn_count_geq_reif") {
    return false;
  }

  const bool isReified = constraintIdentifierIsReified(constraint);

  verifyNumArguments(constraint, isReified ? 4 : 3);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, true)
  if (!isReified) {
    return fzn_count_geq(
        invariantGraph,
        std::get<fznparser::IntVarArray>(constraint.arguments().at(0)),
        std::get<fznparser::IntArg>(constraint.arguments().at(1)),
        std::get<fznparser::IntArg>(constraint.arguments().at(2)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true)
  return fzn_count_geq(
      invariantGraph,
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0)),
      std::get<fznparser::IntArg>(constraint.arguments().at(1)),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn