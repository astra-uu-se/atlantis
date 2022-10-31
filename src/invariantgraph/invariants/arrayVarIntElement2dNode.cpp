#include "invariantgraph/invariants/arrayVarIntElement2dNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::ArrayVarIntElement2dNode>
invariantgraph::ArrayVarIntElement2dNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto idx1 = mappedVariable(constraint.arguments[0], variableMap);
  auto idx2 = mappedVariable(constraint.arguments[1], variableMap);
  auto x = mappedVariableVector(model, constraint.arguments[2], variableMap);
  auto c = mappedVariable(constraint.arguments[3], variableMap);
  const Int numRows = integerValue(model, constraint.arguments[4]);
  assert(numRows > 0);
  assert(x.size() % numRows == 0);
  const Int offset1 = integerValue(model, constraint.arguments[5]);
  assert(offset1 <= idx1->domain().lowerBound());
  const Int offset2 = integerValue(model, constraint.arguments[6]);
  assert(offset2 <= idx2->domain().lowerBound());

  return std::make_unique<invariantgraph::ArrayVarIntElement2dNode>(
      idx1, idx2, x, c, static_cast<size_t>(numRows), offset1, offset2);
}

void invariantgraph::ArrayVarIntElement2dNode::createDefinedVariables(
    Engine& engine) {
  registerDefinedVariable(engine, definedVariables().front());
}

void invariantgraph::ArrayVarIntElement2dNode::registerWithEngine(
    Engine& engine) {
  const size_t numCols = dynamicInputs().size() / _numRows;
  std::vector<std::vector<VarId>> varMatrix(_numRows,
                                            std::vector<VarId>(numCols));
  for (size_t i = 0; i < _numRows; ++i) {
    for (size_t j = 0; j < numCols; ++j) {
      varMatrix.at(i).at(j) = dynamicInputs().at(i * numCols + j)->varId();
    }
  }

  assert(definedVariables().front()->varId() != NULL_ID);
  engine.makeInvariant<Element2dVar>(
      engine, definedVariables().front()->varId(), idx1()->varId(),
      idx2()->varId(), varMatrix, _offset1, _offset2);
}
