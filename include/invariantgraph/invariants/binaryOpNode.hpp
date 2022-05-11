#pragma once

#include <fznparser/model.hpp>

#include "../structure.hpp"

namespace invariantgraph {

/**
 * Invariant that encodes y <- a ⊕ b, where ⊕ is some binary operation.
 */
class BinaryOpNode : public InvariantNode {
 private:
  VariableNode* _y;

 public:
  template <typename T>
  static std::unique_ptr<T> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  BinaryOpNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : InvariantNode({output}, {a, b}), _y{output} {}

  ~BinaryOpNode() override = default;

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

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