#pragma once

#include "../structure.hpp"
#include "fznparser/constraint.hpp"

namespace invariantgraph {

class ArrayIntElementNode : public VariableDefiningNode {
 private:
  std::vector<Int> _as;
  VariableNode* _b;

 public:
  static std::unique_ptr<ArrayIntElementNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  ArrayIntElementNode(std::vector<Int> as, VariableNode* b,
                      VariableNode* output)
      : VariableDefiningNode({output}, {b}), _as(std::move(as)), _b(b) {
    b->imposeDomain({1, static_cast<Int>(_as.size())});

    Int smallest = *std::min_element(_as.begin(), _as.end());
    Int biggest = *std::max_element(_as.begin(), _as.end());
    output->imposeDomain({smallest, biggest});
  }

  void registerWithEngine(
      Engine& engine,
      std::map<VariableNode*, VarId>& variableMap) override;

  [[nodiscard]] const std::vector<Int>& as() const noexcept { return _as; }
  [[nodiscard]] VariableNode* b() const noexcept { return _b; }
};

}  // namespace invariantgraph