#pragma once

#include "fznparser/model.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/forAll.hpp"
#include "views/elementConst.hpp"
#include "views/notEqualView.hpp"

namespace invariantgraph {

class ArrayBoolAndNode : public SoftConstraintNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  ArrayBoolAndNode(std::vector<VariableNode*> as, VariableNode* output)
      : SoftConstraintNode(std::move(as), output) {}

  ArrayBoolAndNode(std::vector<VariableNode*> as, bool shouldHold)
      : SoftConstraintNode(std::move(as), shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_bool_and", 2}};
  }

  static std::unique_ptr<ArrayBoolAndNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};

}  // namespace invariantgraph