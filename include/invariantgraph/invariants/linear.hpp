#pragma once

#include "../structure.hpp"
#include "fznparser/constraint.hpp"

namespace invariantgraph {
class LinearInvariantNode : public InvariantNode {
 private:
  std::vector<Int> _coeffs;
  std::vector<std::shared_ptr<VariableNode>> _variables;
  std::shared_ptr<VariableNode> _output;

 public:
  static LinearInvariantNode fromParser(
      const std::shared_ptr<fznparser::NonFunctionalConstraint>& constraint,
      std::function<
          std::shared_ptr<VariableNode>(const fznparser::ConstraintArgument&)>
          nodeLookup);

  LinearInvariantNode(std::vector<Int> coeffs,
                      std::vector<std::shared_ptr<VariableNode>> vars)
      : _coeffs(std::move(coeffs)), _variables(std::move(vars)) {}

  ~LinearInvariantNode() override = default;

  void registerWithEngine(
      Engine& engine,
      std::function<VarId(const std::shared_ptr<VariableNode>&)> variableMapper)
      const override;
};
}  // namespace invariantgraph
