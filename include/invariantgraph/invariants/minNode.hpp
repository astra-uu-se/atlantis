#pragma once

#include <utility>

#include "../structure.hpp"
#include "fznparser/constraint.hpp"

namespace invariantgraph {
class MinNode : public VariableDefiningNode {
 private:
  std::vector<VariableNode*> _variables;

 public:
  static std::unique_ptr<MinNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

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
      Engine& engine,
      std::map<VariableNode*, VarId>& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& variables() const {
    return _variables;
  }
};
}  // namespace invariantgraph
