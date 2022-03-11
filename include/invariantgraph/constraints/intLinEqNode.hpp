#pragma once

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class IntLinEqNode : public SoftConstraintNode {
 private:
  std::vector<Int> _coeffs;
  std::vector<VariableNode*> _variables;
  Int _c;

 public:
  static std::unique_ptr<IntLinEqNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  IntLinEqNode(std::vector<Int> coeffs, std::vector<VariableNode*> variables,
               Int c)
      : SoftConstraintNode(
            [&] {
              return std::max<Int>(0, std::numeric_limits<Int>::max() - c);
            },
            variables),
        _coeffs(std::move(coeffs)),
        _variables(std::move(variables)),
        _c(c) {}

  void registerWithEngine(
      Engine& engine,
      std::map<VariableNode*, VarId>& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& variables() const {
    return _variables;
  }

  [[nodiscard]] const std::vector<Int>& coeffs() const { return _coeffs; }

  [[nodiscard]] Int c() const { return _c; }

 private:
  [[nodiscard]] VariableNode::Domain getDomainBounds() const;
};

}  // namespace invariantgraph