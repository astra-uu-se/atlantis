#include "invariantgraph/invariants/arrayIntElement2dNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::ArrayIntElement2dNode>
invariantgraph::ArrayIntElement2dNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto idx1 = mappedVariable(constraint.arguments[0], variableMap);
  auto idx2 = mappedVariable(constraint.arguments[1], variableMap);
  auto parVector = integerVector(model, constraint.arguments[2]);
  auto c = mappedVariable(constraint.arguments[3], variableMap);
  const Int numRows = integerValue(model, constraint.arguments[4]);
  assert(numRows > 0);
  assert(parVector.size() % numRows == 0);
  const Int offset1 = integerValue(model, constraint.arguments[5]);
  assert(offset1 <= idx1->domain().lowerBound());
  const Int offset2 = integerValue(model, constraint.arguments[6]);
  assert(offset2 <= idx2->domain().lowerBound());

  const size_t numCols = parVector.size() / static_cast<size_t>(numRows);

  std::vector<std::vector<Int>> parMatrix(numRows, std::vector<Int>(numCols));
  for (Int i = 0; i < numRows; ++i) {
    for (size_t j = 0; j < numCols; ++j) {
      parMatrix.at(i).at(j) = parVector.at(i * numCols + j);
    }
  }

  return std::make_unique<invariantgraph::ArrayIntElement2dNode>(
      idx1, idx2, parMatrix, c, offset1, offset2);
}

void invariantgraph::ArrayIntElement2dNode::createDefinedVariables(
    Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void invariantgraph::ArrayIntElement2dNode::registerWithEngine(Engine& engine) {
  assert(definedVariables().front()->varId() != NULL_ID);
  engine.makeInvariant<Element2dConst>(definedVariables().front()->varId(),
                                       idx1()->varId(), idx2()->varId(),
                                       _parMatrix, _offset1, _offset2);
}
