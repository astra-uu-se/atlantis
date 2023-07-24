#pragma once

#include <cmath>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/variableDefiningNode.hpp"
#include "invariants/plus.hpp"

namespace invariantgraph {

class IntPlusNode : public VariableDefiningNode {
 public:
  IntPlusNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : VariableDefiningNode({output}, {a, b}) {
    assert(std::all_of(
        staticInputs().begin(), staticInputs().end(),
        [&](auto* const staticInput) { return staticInput->isIntVar(); }));
  }

  ~IntPlusNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_plus", 3}};
  }

  static std::unique_ptr<IntPlusNode> fromModelConstraint(
      const fznparser::Model& model, const fznparser::Constraint& constraint,
      InvariantGraph& invariantGraph);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] VariableNode* a() const noexcept {
    return staticInputs().front();
  }
  [[nodiscard]] VariableNode* b() const noexcept {
    return staticInputs().back();
  }
};

}  // namespace invariantgraph