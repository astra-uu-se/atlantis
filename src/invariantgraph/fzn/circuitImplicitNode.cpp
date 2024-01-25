

#include "invariantgraph/fzn/circuitImplicitNode.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool makeCircuitImplicitNode(FznInvariantGraph& invariantGraph,
                             const fznparser::IntVarArray& inputs) {
  if (inputs.size() < 2) {
    return true;
  }

  for (size_t i = 0; i < inputs.size(); ++i) {
    if (std::holds_alternative<Int>(inputs.at(i))) {
      return false;
    }
  }

  // For now, this only works when all the vars have the same domain.
  const fznparser::IntSet& domain =
      std::get<std::reference_wrapper<const fznparser::IntVar>>(inputs.at(0))
          .get()
          .domain();

  for (size_t i = 1; i < inputs.size(); ++i) {
    if (domain !=
        std::get<std::reference_wrapper<const fznparser::IntVar>>(inputs.at(i))
            .get()
            .domain()) {
      return false;
    }
  }
  invariantGraph.addImplicitConstraintNode(
      std::move(std::make_unique<CircuitImplicitNode>(
          std::move(invariantGraph.createVarNodes(inputs, true)))));
  return true;
}

bool makeCircuitImplicitNode(FznInvariantGraph& invariantGraph,
                             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_circuit") {
    return false;
  }

  verifyNumArguments(constraint, 1);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true);

  return makeCircuitImplicitNode(
      invariantGraph,
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0)));
}

}  // namespace atlantis::invariantgraph::fzn