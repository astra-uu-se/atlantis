#include "invariantgraph/implicitConstraints/circuitImplicitNode.hpp"

#include "../parseHelper.hpp"
#include "search/neighbourhoods/circuitNeighbourhood.hpp"

namespace invariantgraph {

std::unique_ptr<CircuitImplicitNode> CircuitImplicitNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntVarArray& arg =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0));

  if (arg.size() < 2) {
    return nullptr;
  }

  for (size_t i = 0; i < arg.size(); ++i) {
    if (std::holds_alternative<Int>(arg.at(i))) {
      return nullptr;
    }
  }

  // For now, this only works when all the variables have the same domain.
  const fznparser::IntSet& domain =
      std::get<std::reference_wrapper<const fznparser::IntVar>>(arg.at(0))
          .get()
          .domain();

  for (size_t i = 1; i < arg.size(); ++i) {
    if (domain !=
        std::get<std::reference_wrapper<const fznparser::IntVar>>(arg.at(i))
            .get()
            .domain()) {
      return nullptr;
    }
  }

  return std::make_unique<CircuitImplicitNode>(
      invariantGraph.addVariableArray(arg));
}

CircuitImplicitNode::CircuitImplicitNode(std::vector<VariableNode*> variables)
    : ImplicitConstraintNode(std::move(variables)) {
  assert(definedVariables().size() > 1);

  assert(std::all_of(definedVariables().begin(), definedVariables().end(),
                     [&](VariableNode* const variable) {
                       return variable->domain() ==
                              definedVariables().front()->domain();
                     }));
}

search::neighbourhoods::Neighbourhood* CircuitImplicitNode::createNeighbourhood(
    Engine&, std::vector<search::SearchVariable> variables) {
  return new search::neighbourhoods::CircuitNeighbourhood(variables);
}

}  // namespace invariantgraph