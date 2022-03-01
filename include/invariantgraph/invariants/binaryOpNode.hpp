#pragma once

#include "../structure.hpp"

namespace invariantgraph {

/**
 * Invariant that encodes y <- a ⊕ b, where ⊕ is some binary operation.
 */
class BinaryOpNode : public InvariantNode {
 private:
  VariableNode* _a;
  VariableNode* _b;

 public:
  template <typename T>
  static std::unique_ptr<T> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  BinaryOpNode(VariableNode* a, VariableNode* b, VariableNode* output)
      : InvariantNode(output), _a(a), _b(b) {}

  ~BinaryOpNode() override = default;

  void registerWithEngine(
      Engine& engine,
      std::function<VarId(VariableNode*)> variableMapper) const override;

  [[nodiscard]] VariableNode* a() const noexcept { return _a; }
  [[nodiscard]] VariableNode* b() const noexcept { return _b; }

 protected:
  virtual void createInvariant(Engine& engine, VarId a, VarId b,
                               VarId output) const = 0;
};

}  // namespace invariantgraph