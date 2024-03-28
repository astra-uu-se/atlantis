#include "atlantis/invariantgraph/fzn/circuitImplicitNode.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/circuitImplicitNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool makeCircuitImplicitNode(
    FznInvariantGraph& invariantGraph,
    const std::shared_ptr<fznparser::IntVarArray>& inputs) {
  if (inputs->size() < 2) {
    return true;
  }

  for (size_t i = 0; i < inputs->size(); ++i) {
    if (std::holds_alternative<Int>(inputs->at(i))) {
      return false;
    }
  }

  // For now, this only works when all the vars have the same domain.
  const fznparser::IntSet& domain =
      std::get<std::shared_ptr<const fznparser::IntVar>>(inputs->at(0))
          ->domain();

  for (size_t i = 1; i < inputs->size(); ++i) {
    if (domain !=
        std::get<std::shared_ptr<const fznparser::IntVar>>(inputs->at(i))
            ->domain()) {
      return false;
    }
  }
  invariantGraph.addImplicitConstraintNode(
      std::make_unique<CircuitImplicitNode>(
          invariantGraph.retrieveVarNodes(inputs)));
  return true;
}

bool makeCircuitImplicitNode(FznInvariantGraph& invariantGraph,
                             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_circuit") {
    return false;
  }

  verifyNumArguments(constraint, 1);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)

  return makeCircuitImplicitNode(
      invariantGraph, std::get<std::shared_ptr<fznparser::IntVarArray>>(
                          constraint.arguments().at(0)));
}

}  // namespace atlantis::invariantgraph::fzn
