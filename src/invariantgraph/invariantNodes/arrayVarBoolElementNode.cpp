#include "invariantgraph/invariantNodes/arrayVarBoolElementNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

ArrayVarBoolElementNode::ArrayVarBoolElementNode(VarNodeId b,
                                                 std::vector<VarNodeId>&& as,
                                                 VarNodeId output, Int offset)
    : InvariantNode({output}, {b}, std::move(as)), _offset(offset) {}

std::unique_ptr<ArrayVarBoolElementNode>
ArrayVarBoolElementNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg b =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));

  // Compute offset if nonshifted variant:
  Int offset = constraint.identifier() != "array_var_bool_element_nonshifted"
                   ? 1
                   : (b.isParameter() ? b.parameter() : b.var().lowerBound());

  const fznparser::BoolVarArray as =
      std::get<fznparser::BoolVarArray>(constraint.arguments().at(1));

  const fznparser::BoolArg c =
      std::get<fznparser::BoolArg>(constraint.arguments().at(2));

  return std::make_unique<ArrayVarBoolElementNode>(
      invariantGraph.createVarNode(b), invariantGraph.createVarNodes(as),
      invariantGraph.createVarNode(c), offset);
}

void ArrayVarBoolElementNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  // TODO: offset can be different than 1
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()), 1);
}

void ArrayVarBoolElementNode::registerNode(InvariantGraph& invariantGraph,
                                           propagation::SolverBase& solver) {
  std::vector<propagation::VarId> as;
  std::transform(dynamicInputVarNodeIds().begin(),
                 dynamicInputVarNodeIds().end(), std::back_inserter(as),
                 [&](auto node) { return invariantGraph.varId(node); });

  assert(invariantGraph.varId(outputVarNodeIds().front()) != propagation::NULL_ID);

  solver.makeInvariant<propagation::ElementVar>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(b()), as);
}

}  // namespace invariantgraph