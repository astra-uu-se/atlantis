

#include "invariantgraph/fzn/int_lt.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_lt(FznInvariantGraph& invariantGraph,
            const fznparser::Constraint& constraint) {
  // assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 &&
      constraint.arguments().size() != 3) {
    throw std::runtime_error("IntLe constraint takes two var bool arguments");
  }

  VarNodeId a = invariantGraph.createVarNodeFromFzn(
      std::get<fznparser::IntArg>(constraint.arguments().at(0)), false);

  VarNodeId b = invariantGraph.createVarNodeFromFzn(
      std::get<fznparser::IntArg>(constraint.arguments().at(1)), false);

  if (constraint.arguments().size() == 2) {
    invariantGraph.addInvariantNode(
        std::move(std::make_unique<IntLtNode>(a, b, true)));
    return true;
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (reified.isFixed()) {
    invariantGraph.addInvariantNode(
        std::move(std::make_unique<IntLtNode>(a, b, reified.toParameter())));
    return true;
  }
  invariantGraph.addInvariantNode(std::move(std::make_unique<IntLtNode>(
      a, b, invariantGraph.createVarNodeFromFzn(reified.var(), true))));
  return true;
}

}  // namespace atlantis::invariantgraph::fzn