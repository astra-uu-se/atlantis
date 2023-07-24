#include "invariantgraph/invariants/arrayIntElementNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<ArrayIntElementNode> ArrayIntElementNode::fromModelConstraint(
    const fznparser::Model& model, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntVar& idx =
      std::get<std::reference_wrapper<const fznparser::IntVar>>(
          std::get<fznparser::IntArg>(constraint.arguments().at(0)));

  std::vector<Int> as =
      std::get<fznParser::IntVarArray>(constraint.arguments().at(1))
          .toParVector();

  const fznparser::IntVar& c =
      std::get<std::reference_wrapper<const fznparser::IntVar>>(
          std::get<fznparser::BoolArg>(constraint.arguments().at(2)));

  const Int offset =
      constraint.name != "array_int_element_offset" ? 1 : idx.lowerBound();

  return std::make_unique<ArrayIntElementNode>(
      as, invariantGraph.addVariable(idx), invariantGraph.addVariable(c),
      offset);
}

void ArrayIntElementNode::createDefinedVariables(Engine& engine) {
  if (definedVariables().front()->varId() == NULL_ID) {
    assert(b()->varId() != NULL_ID);
    definedVariables().front()->setVarId(
        engine.makeIntView<ElementConst>(engine, b()->varId(), _as, _offset));
  }
}

void ArrayIntElementNode::registerWithEngine(Engine&) {}

}  // namespace invariantgraph