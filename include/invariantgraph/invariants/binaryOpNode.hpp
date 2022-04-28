#pragma once

#include <fznparser/model.hpp>

#include "../structure.hpp"

namespace invariantgraph {

/**
 * Invariant that encodes y <- a ⊕ b, where ⊕ is some binary operation.
 */
class BinaryOpNode : public VariableDefiningNode {
 private:
  VariableNode* _a;
  VariableNode* _b;

 public:
  template <typename T>
  static std::unique_ptr<T> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  BinaryOpNode(VariableNode* a, VariableNode* b, VariableNode* output,
               bool isView)
      : VariableDefiningNode({output}, isView, {a, b}), _a(a), _b(b) {}

  ~BinaryOpNode() override = default;

  void createDefinedVariables(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] VariableNode* a() const noexcept { return _a; }
  [[nodiscard]] VariableNode* b() const noexcept { return _b; }

 protected:
  virtual void createInvariant(Engine& engine, VarId a, VarId b,
                               VarId output) const = 0;
};

}  // namespace invariantgraph