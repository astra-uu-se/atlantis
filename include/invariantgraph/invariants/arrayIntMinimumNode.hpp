#pragma once

#include <algorithm>
#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/variableDefiningNode.hpp"
#include "invariants/minSparse.hpp"

namespace invariantgraph {
class ArrayIntMinimumNode : public VariableDefiningNode {
 public:
  ArrayIntMinimumNode(std::vector<VariableNode*> variables,
                      VariableNode* output)
      : VariableDefiningNode({output}, variables) {
    assert(std::all_of(
        staticInputs().begin(), staticInputs().end(),
        [&](auto* const staticInput) { return staticInput->isIntVar(); }));
  }

  ~ArrayIntMinimumNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_int_minimum", 2}};
  }

  static std::unique_ptr<ArrayIntMinimumNode> fromModelConstraint(
      const fznparser::Model& model, const fznparser::Constraint& constraint,
      InvariantGraph& invariantGraph);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};
}  // namespace invariantgraph
