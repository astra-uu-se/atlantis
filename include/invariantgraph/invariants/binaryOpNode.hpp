#pragma once

#include <fznparser/model.hpp>

#include "../structure.hpp"

namespace invariantgraph {

/**
 * Invariant that encodes y <- a ⊕ b, where ⊕ is some binary operation.
 */
class BinaryOpNode : public VariableDefiningNode {
 public:
  template <typename T>
  static std::unique_ptr<T> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  BinaryOpNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : VariableDefiningNode({output}, {a, b}) {}

  ~BinaryOpNode() override = default;

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] VariableNode* a() const noexcept {
    return staticInputs().front();
  }
  [[nodiscard]] VariableNode* b() const noexcept {
    return staticInputs().back();
  }

 protected:
  virtual void createInvariant(Engine& engine, VarId a, VarId b,
                               VarId output) const = 0;
};

}  // namespace invariantgraph