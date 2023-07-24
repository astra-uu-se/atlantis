#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/equal.hpp"
#include "constraints/notEqual.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/softConstraintNode.hpp"

namespace invariantgraph {

class IntEqNode : public SoftConstraintNode {
 public:
  explicit IntEqNode(VariableNode* a, VariableNode* b, VariableNode* r)
      : SoftConstraintNode({a, b}, r) {}
  explicit IntEqNode(VariableNode* a, VariableNode* b, bool shouldHold = true)
      : SoftConstraintNode({a, b}, shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_eq", 2},
                                                            {"int_eq_reif", 3}};
  }

  static std::unique_ptr<IntEqNode> fromModelConstraint(
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