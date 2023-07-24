#pragma once

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/variableDefiningNode.hpp"
#include "invariants/intDiv.hpp"

namespace invariantgraph {

class IntDivNode : public VariableDefiningNode {
 public:
  IntDivNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : VariableDefiningNode({output}, {a, b}) {
    assert(std::all_of(
        staticInputs().begin(), staticInputs().end(),
        [&](auto* const staticInput) { return staticInput->isIntVar(); }));
  }

  ~IntDivNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_div", 3}};
  }

  static std::unique_ptr<IntDivNode> fromModelConstraint(
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