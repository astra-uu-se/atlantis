#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/boolLessEqual.hpp"
#include "constraints/boolLessThan.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/softConstraintNode.hpp"

namespace invariantgraph {

class BoolLeNode : public SoftConstraintNode {
 public:
  BoolLeNode(VariableNode* a, VariableNode* b, VariableNode* r)
      : SoftConstraintNode({a, b}, r) {}
  BoolLeNode(VariableNode* a, VariableNode* b, bool shouldHold)
      : SoftConstraintNode({a, b}, shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"bool_le", 2}, {"bool_le_reif", 3}};
  }

  static std::unique_ptr<BoolLeNode> fromModelConstraint(
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