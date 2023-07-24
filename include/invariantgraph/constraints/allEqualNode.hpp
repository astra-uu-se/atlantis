#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/allDifferent.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "views/equalConst.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {
class InvariantGraph;  // Forward declaration

class AllEqualNode : public SoftConstraintNode {
 private:
  VarId _allDifferentViolationVarId{NULL_ID};

 public:
  explicit AllEqualNode(std::vector<VariableNode*> variables, VariableNode* r)
      : SoftConstraintNode(variables, r) {}

  explicit AllEqualNode(std::vector<VariableNode*> variables,
                        bool shouldHold = true)
      : SoftConstraintNode(variables, shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"fzn_all_equal_int", 1}, {"fzn_all_equal_int_reif", 2}};
  }

  static std::unique_ptr<AllEqualNode> fromModelConstraint(
      const fznparser::Model& model, const fznparser::Constraint& constraint,
      InvariantGraph& invariantGraph);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};
}  // namespace invariantgraph