#include "invariantgraph/invariantNodes/arrayIntElement2dNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

ArrayIntElement2dNode::ArrayIntElement2dNode(
    VarNodeId idx1, VarNodeId idx2, std::vector<std::vector<Int>>&& parMatrix,
    VarNodeId output, Int offset1, Int offset2)
    : InvariantNode({output}, {idx1, idx2}),
      _parMatrix(std::move(parMatrix)),
      _offset1(offset1),
      _offset2(offset2) {}

std::unique_ptr<ArrayIntElement2dNode>
ArrayIntElement2dNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg& idx1 =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));

  const fznparser::IntArg& idx2 =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));

  std::vector<Int> parVector =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(2))
          .toParVector();

  const fznparser::IntArg& cArg =
      std::get<fznparser::IntArg>(constraint.arguments().at(3));

  const Int numRows =
      std::get<fznparser::IntArg>(constraint.arguments().at(4)).toParameter();

  assert(numRows > 0);
  assert(parVector.size() % numRows == 0);

  const Int offset1 =
      std::get<fznparser::IntArg>(constraint.arguments().at(5)).toParameter();
  assert(offset1 <=
         (idx1.isParameter() ? idx1.parameter() : idx1.var().lowerBound()));

  const Int offset2 =
      std::get<fznparser::IntArg>(constraint.arguments().at(6)).toParameter();
  assert(offset2 <=
         (idx2.isParameter() ? idx2.parameter() : idx2.var().lowerBound()));

  const size_t numCols = parVector.size() / static_cast<size_t>(numRows);

  std::vector<std::vector<Int>> parMatrix(numRows, std::vector<Int>(numCols));
  for (Int i = 0; i < numRows; ++i) {
    for (size_t j = 0; j < numCols; ++j) {
      parMatrix.at(i).at(j) = parVector.at(i * numCols + j);
    }
  }

  return std::make_unique<ArrayIntElement2dNode>(
      invariantGraph.createVarNode(idx1), invariantGraph.createVarNode(idx2),
      std::move(parMatrix), invariantGraph.createVarNode(cArg), offset1,
      offset2);
}

void ArrayIntElement2dNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  makeEngineVar(engine, invariantGraph.varNode(outputVarNodeIds().front()));
}

void ArrayIntElement2dNode::registerNode(InvariantGraph& invariantGraph,
                                         Engine& engine) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) != NULL_ID);
  engine.makeInvariant<Element2dConst>(
      engine, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(idx1()), invariantGraph.varId(idx2()), _parMatrix,
      _offset1, _offset2);
}

}  // namespace invariantgraph