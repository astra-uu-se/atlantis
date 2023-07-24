#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/boolEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/boolXor.hpp"

namespace invariantgraph {

class BoolEqNode : public SoftConstraintNode {
 public:
  BoolEqNode(VariableNode* a, VariableNode* b, VariableNode* r = nullptr)
      : SoftConstraintNode({a, b}, r) {}
  BoolEqNode(VariableNode* a, VariableNode* b, bool shouldHold)
      : SoftConstraintNode({a, b}, shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"bool_eq", 2}, {"bool_eq_reif", 3}};
  }

  static std::unique_ptr<BoolEqNode> fromModelConstraint(
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