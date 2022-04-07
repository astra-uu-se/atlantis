#pragma once

#include <utility>

#include "invariantgraph/structure.hpp"

namespace invariantgraph {
class SetInNode : public SoftConstraintNode {
 private:
  VariableNode* _input;
  std::vector<Int> _values;

 public:
  explicit SetInNode(VariableNode* input, std::vector<Int> values)
      // TODO: Specify a better upper bound.
      : SoftConstraintNode([&] { return 1; }, {input}),
        _input(input),
        _values(std::move(values)) {}

  static std::unique_ptr<SetInNode> fromModelConstraint(
      const std::shared_ptr<fznparser::Constraint>& constraint,
      const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
          variableMap);

  void registerWithEngine(Engine& engine,
                          std::map<VariableNode*, VarId>& variableMap) override;

  [[nodiscard]] const std::vector<Int>& values() { return _values; }
};
}  // namespace invariantgraph