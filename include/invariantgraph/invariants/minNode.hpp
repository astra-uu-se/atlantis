#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "../structure.hpp"

namespace invariantgraph {
class MinNode : public VariableDefiningNode {
 private:
  std::vector<VariableNode*> _variables;

 public:
  static std::unique_ptr<MinNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  MinNode(std::vector<VariableNode*> variables, VariableNode* output)
      : VariableDefiningNode({output}, variables),
        _variables(std::move(variables)) {
    Int outputLb = std::numeric_limits<Int>::max();
    Int outputUb = std::numeric_limits<Int>::max();

    for (const auto& node : _variables) {
      const auto& [nodeLb, nodeUb] = node->bounds();
      outputLb = std::min(nodeLb, outputLb);
      outputUb = std::min(nodeUb, outputUb);
    }

    output->imposeDomain(IntervalDomain{outputLb, outputUb});
  }

  ~MinNode() override = default;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& variables() const {
    return _variables;
  }
};
}  // namespace invariantgraph
