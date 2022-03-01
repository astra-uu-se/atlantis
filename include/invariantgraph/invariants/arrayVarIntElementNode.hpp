#pragma once

#include "../structure.hpp"
#include "fznparser/constraint.hpp"

namespace invariantgraph {

class ArrayVarIntElementNode : public InvariantNode {
 private:
  std::vector<VariableNode*> _as;
  VariableNode* _b;

 public:
  static std::unique_ptr<ArrayVarIntElementNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  ArrayVarIntElementNode(std::vector<VariableNode*> as,
                                  VariableNode* b, VariableNode* output)
      : InvariantNode(output), _as(std::move(as)), _b(b) {}

  void registerWithEngine(
      Engine& engine,
      std::function<VarId(VariableNode*)> variableMapper) const override;

  [[nodiscard]] const std::vector<VariableNode*>& as() const noexcept {
    return _as;
  }
  [[nodiscard]] VariableNode* b() const noexcept { return _b; }
};

}  // namespace invariantgraph