#pragma once
#include <fznparser/model.hpp>
#include <utility>

#include "constraints/inDomain.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "utils/variant.hpp"
#include "views/notEqualView.hpp"

namespace invariantgraph {
class SetInNode : public SoftConstraintNode {
 private:
  std::vector<Int> _values;
  VarId _intermediate{NULL_ID};

 public:
  explicit SetInNode(VariableNode* input, std::vector<Int> values,
                     VariableNode* r)
      : SoftConstraintNode({input}, r), _values(std::move(values)) {}
  explicit SetInNode(VariableNode* input, std::vector<Int> values,
                     bool shouldHold)
      : SoftConstraintNode({input}, shouldHold), _values(std::move(values)) {}

  static std::unique_ptr<SetInNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] const std::vector<Int>& values() { return _values; }
};
}  // namespace invariantgraph