#include "invariantgraph/invariants/arrayIntElement2dNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<invariantgraph::ArrayIntElement2dNode>
invariantgraph::ArrayIntElement2dNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg& argIdx1 =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));
  const fznparser::IntArg& argIdx2 =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));

  std::vector<Int> parVector =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(2))
          .toParVector();

  const fznparser::IntVar& c =
      std::get<std::reference_wrapper<const IntVar>>(
          std::get<fznparser::BoolArg>(constraint.arguments().at(3)))
          .get();

  const Int numRows =
      std::get<fznparser::IntArg>(constraint.arguments().at(4)).toParameter();
  assert(numRows > 0);
  assert(parVector.size() % numRows == 0);

  const Int offset1 =
      std::get<fznparser::IntArg>(constraint.arguments().at(5)).toParameter();
  assert(offset1 <= idx1.lowerBound());

  const Int offset2 =
      std::get<fznparser::IntArg>(constraint.arguments().at(6)).toParameter();
  assert(offset2 <= idx2.lowerBound());

  const size_t numCols = parVector.size() / static_cast<size_t>(numRows);

  std::vector<std::vector<Int>> parMatrix(numRows, std::vector<Int>(numCols));
  for (Int i = 0; i < numRows; ++i) {
    for (size_t j = 0; j < numCols; ++j) {
      parMatrix.at(i).at(j) = parVector.at(i * numCols + j);
    }
  }

  return std::make_unique<invariantgraph::ArrayIntElement2dNode>(
      invariantGraph.addVariable(
          std::get<std::reference_wrapper<const IntVar>>(argIdx1).get()),
      invariantGraph.addVariable(
          std::get<std::reference_wrapper<const IntVar>>(argIdx2).get()),
      parMatrix, invariantGraph.addVariable(c), offset1, offset2);
}

void invariantgraph::ArrayIntElement2dNode::createDefinedVariables(
    Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void invariantgraph::ArrayIntElement2dNode::registerWithEngine(Engine& engine) {
  assert(definedVariables().front()->varId() != NULL_ID);
  engine.makeInvariant<Element2dConst>(
      engine, definedVariables().front()->varId(), idx1()->varId(),
      idx2()->varId(), _parMatrix, _offset1, _offset2);
}

}  // namespace invariantgraph