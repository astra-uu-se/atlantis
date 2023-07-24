#pragma once

#include <utility>

#include "fznparser/model.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/boolLinear.hpp"
#include "views/equalConst.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {

class ArrayBoolXorNode : public SoftConstraintNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  ArrayBoolXorNode(std::vector<VariableNode*>&& as, VariableNode* output)
      : SoftConstraintNode(std::move(as), output) {}

  ArrayBoolXorNode(std::vector<VariableNode*>&& as, bool shouldHold)
      : SoftConstraintNode(std::move(as), shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"array_bool_xor", 1}};
  }

  static std::unique_ptr<ArrayBoolXorNode> fromModelConstraint(
      const fznparser::Model& model, const fznparser::Constraint& constraint,
      InvariantGraph& invariantGraph);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};

}  // namespace invariantgraph