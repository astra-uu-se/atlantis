#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/allDifferent.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {
class AllDifferentNode : public SoftConstraintNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  explicit AllDifferentNode(std::vector<VariableNode*>&& variables,
                            VariableNode* r);

  explicit AllDifferentNode(std::vector<VariableNode*>&& variables,
                            bool shouldHold);

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"fzn_all_different_int", 1}, {"fzn_all_different_int_reif", 2}};
  }

  static std::unique_ptr<AllDifferentNode> fromModelConstraint(
      const fznparser::Model& model, const fznparser::Constraint& constraint,
      InvariantGraph& invariantGraph);

  bool prune();

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};
}  // namespace invariantgraph