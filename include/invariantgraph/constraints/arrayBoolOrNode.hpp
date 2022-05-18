#pragma once

#include <utility>

#include "fznparser/model.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/exists.hpp"
#include "views/notEqualView.hpp"

namespace invariantgraph {

class ArrayBoolOrNode : public SoftConstraintNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  static std::unique_ptr<ArrayBoolOrNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayBoolOrNode(std::vector<VariableNode*> as, VariableNode* output)
      : SoftConstraintNode(std::move(as), output) {}

  ArrayBoolOrNode(std::vector<VariableNode*> as, bool shouldHold)
      : SoftConstraintNode(std::move(as), shouldHold) {}

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};

}  // namespace invariantgraph