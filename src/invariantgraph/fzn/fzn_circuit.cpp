#include "atlantis/invariantgraph/fzn/fzn_circuit.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/circuitNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_circuit(FznInvariantGraph& graph,
                 const std::shared_ptr<fznparser::IntVarArray>& inputs) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();
  for (size_t i = 0; i < inputs->size(); ++i) {
    if (std::holds_alternative<Int>(inputs->at(i))) {
      lb = std::min(lb, std::get<Int>(inputs->at(i)));
      ub = std::max(lb, std::get<Int>(inputs->at(i)));
    } else {
      const auto var =
          std::get<std::shared_ptr<const fznparser::IntVar>>(inputs->at(i));
      lb = std::min(lb, var->lowerBound());
      ub = std::max(lb, var->upperBound());
    }
  }
  if (ub - lb != static_cast<Int>(inputs->size())) {
    throw FznArgumentException(
        "Constraint fzn_circuit: the bounds for the variables are not valid. "
        "The greatest upper bound subtracted by the smallest lower bound "
        "should equal the size of the input array (" +
        std::to_string(ub) + " - " + std::to_string(lb) +
        " != " + std::to_string(inputs->size()) + ")");
  }
  return fzn_circuit(graph, inputs, lb);
}

bool fzn_circuit(FznInvariantGraph& graph,
                 const std::shared_ptr<fznparser::IntVarArray>& inputs,
                 Int offset) {
  graph.addInvariantNode(std::make_shared<CircuitNode>(
      graph, graph.retrieveVarNodes(inputs), offset));
  return true;
}

bool fzn_circuit(FznInvariantGraph& graph,
                 const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_circuit" &&
      constraint.identifier() != "fzn_circuit_offset") {
    return false;
  }

  bool hasOffset = constraint.identifier() == "fzn_circuit_offset";

  verifyNumArguments(constraint, hasOffset ? 2 : 1);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)
  if (hasOffset) {
    FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, false)
    return fzn_circuit(
        graph,
        getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)),
        std::get<fznparser::IntArg>(constraint.arguments().at(1))
            .toParameter());
  }
  return fzn_circuit(
      graph, getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)));
}

}  // namespace atlantis::invariantgraph::fzn
