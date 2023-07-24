#pragma once

#include <fznparser/model.hpp>
#include <map>
#include <utility>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/variableDefiningNode.hpp"

namespace invariantgraph {

class IntAbsNode : public VariableDefiningNode {
 public:
  IntAbsNode(VariableNode* staticInput, VariableNode* output)
      : VariableDefiningNode({output}, {staticInput}) {}

  ~IntAbsNode() override = default;

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{{"int_abs", 2}};
  }

  static std::unique_ptr<IntAbsNode> fromModelConstraint(
      const fznparser::Model& model, const fznparser::Constraint& constraint,
      InvariantGraph& invariantGraph);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] VariableNode* input() const noexcept {
    return staticInputs().front();
  }
};

}  // namespace invariantgraph