#include "invariantgraph/invariantNodes/arrayVarIntElement2dNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

ArrayVarIntElement2dNode::ArrayVarIntElement2dNode(
    VarNodeId idx1, VarNodeId idx2, std::vector<VarNodeId>&& as,
    VarNodeId output, size_t numRows, Int offset1, Int offset2)
    : InvariantNode({output}, {idx1, idx2}, std::move(as)),
      _numRows(numRows),
      _offset1(offset1),
      _offset2(offset2) {}

std::unique_ptr<ArrayVarIntElement2dNode>
ArrayVarIntElement2dNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg& idx1 =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));

  const fznparser::IntArg& idx2 =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));

  const fznparser::IntVarArray& x =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(2));

  const fznparser::IntArg& c =
      std::get<fznparser::IntArg>(constraint.arguments().at(3));

  const Int numRows =
      std::get<fznparser::IntArg>(constraint.arguments().at(4)).parameter();

  assert(numRows > 0);
  assert(x.size() % numRows == 0);

  const Int offset1 =
      std::get<fznparser::IntArg>(constraint.arguments().at(5)).parameter();
  assert(offset1 <=
         (idx1.isParameter() ? idx1.parameter() : idx1.var().lowerBound()));

  const Int offset2 =
      std::get<fznparser::IntArg>(constraint.arguments().at(6)).parameter();
  assert(offset2 <=
         (idx2.isParameter() ? idx2.parameter() : idx2.var().lowerBound()));

  return std::make_unique<ArrayVarIntElement2dNode>(
      invariantGraph.createVarNode(idx1), invariantGraph.createVarNode(idx2),
      invariantGraph.createVarNodes(x), invariantGraph.createVarNode(c),
      static_cast<size_t>(numRows), offset1, offset2);
}

void ArrayVarIntElement2dNode::registerOutputVariables(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void ArrayVarIntElement2dNode::registerNode(InvariantGraph& invariantGraph,
                                            propagation::SolverBase& solver) {
  const size_t numCols = dynamicInputVarNodeIds().size() / _numRows;
  std::vector<std::vector<propagation::VarId>> varMatrix(_numRows,
                                            std::vector<propagation::VarId>(numCols));
  for (size_t i = 0; i < _numRows; ++i) {
    for (size_t j = 0; j < numCols; ++j) {
      varMatrix.at(i).at(j) =
          invariantGraph.varNode(dynamicInputVarNodeIds().at(i * numCols + j))
              .varId();
    }
  }

  assert(invariantGraph.varId(outputVarNodeIds().front()) != propagation::NULL_ID);
  solver.makeInvariant<propagation::Element2dVar>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(idx1()), invariantGraph.varId(idx2()), varMatrix,
      _offset1, _offset2);
}

}  // namespace invariantgraph