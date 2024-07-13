#include "atlantis/invariantgraph/fzn/fzn_circuit.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/circuitNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_circuit(FznInvariantGraph& invariantGraph,
                 const std::shared_ptr<fznparser::IntVarArray>& inputs) {
  invariantGraph.addInvariantNode(
      std::make_unique<CircuitNode>(invariantGraph.retrieveVarNodes(inputs)));
  return true;
}

bool fzn_circuit(FznInvariantGraph& invariantGraph,
                 const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "circuit_no_offset") {
    return false;
  }

  verifyNumArguments(constraint, 1);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)

  return fzn_circuit(invariantGraph, getArgArray<fznparser::IntVarArray>(
                                         constraint.arguments().at(0)));
}

}  // namespace atlantis::invariantgraph::fzn
