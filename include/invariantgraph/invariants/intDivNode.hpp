#pragma once

#include "../structure.hpp"

namespace invariantgraph {

class IntDivNode : public InvariantNode {
 private:
  VariableNode* _a;
  VariableNode* _b;

 public:
  static std::unique_ptr<IntDivNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  IntDivNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : InvariantNode(output), _a(a), _b(b) {}

  ~IntDivNode() override = default;

  void registerWithEngine(
      Engine& engine,
      std::function<VarId(VariableNode*)> variableMapper) const override;

  [[nodiscard]] VariableNode* a() const noexcept { return _a; }
  [[nodiscard]] VariableNode* b() const noexcept { return _b; }
};

}