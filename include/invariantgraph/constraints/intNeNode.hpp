#pragma once

#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class IntNeNode : public SoftConstraintNode {
 private:
  VariableNode* _a;
  VariableNode* _b;

 public:
  IntNeNode(VariableNode* a, VariableNode* b)
      : SoftConstraintNode([] { return 1; }, {a, b}), _a(a), _b(b) {}

  static std::unique_ptr<IntNeNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  void registerWithEngine(
      Engine& engine,
      std::map<VariableNode*, VarId>& variableMap) override;

  [[nodiscard]] VariableNode* a() const noexcept { return _a; }
  [[nodiscard]] VariableNode* b() const noexcept { return _b; }
};

}  // namespace invariantgraph