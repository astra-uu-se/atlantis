#include "atlantis/invariantgraph/fzn/fzn_circuit.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/circuitNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_circuit(FznInvariantGraph& graph,
                 const std::shared_ptr<fznparser::IntVarArray>& inputs) {
  graph.addInvariantNode(
      std::make_unique<CircuitNode>(graph.retrieveVarNodes(inputs)));
  return true;
}

bool fzn_circuit(FznInvariantGraph& graph,
                 const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "circuit_no_offset") {
    return false;
  }

  verifyNumArguments(constraint, 1);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)

  return fzn_circuit(
      graph, getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)));
}

}  // namespace atlantis::invariantgraph::fzn
