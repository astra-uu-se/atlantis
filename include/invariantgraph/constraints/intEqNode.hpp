#pragma once

#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {

class IntEqNode : public SoftConstraintNode {
 private:
  VariableNode* _a;
  VariableNode* _b;

  inline static auto violationUb = [](auto a, auto b) {
    auto al = a->variable()->domain()->lowerBound();
    auto au = a->variable()->domain()->upperBound();
    auto bl = b->variable()->domain()->lowerBound();
    auto bu = b->variable()->domain()->upperBound();

    auto diff1 = std::abs(al - bu);
    auto diff2 = std::abs(au - bl);

    return std::max(diff1, diff2);
  };

 public:
  IntEqNode(VariableNode* a, VariableNode* b)
      : SoftConstraintNode([&] { return violationUb(a, b); }, {a, b}),
        _a(a),
        _b(b) {}

  static std::unique_ptr<IntEqNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  void registerWithEngine(
      Engine& engine,
      std::map<VariableNode *, VarId> &variableMap) override;

  [[nodiscard]] VariableNode* a() const noexcept { return _a; }
  [[nodiscard]] VariableNode* b() const noexcept { return _b; }
};

}  // namespace invariantgraph