#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "propagation/constraints/allDifferent.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/violationInvariantNode.hpp"
#include "propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {
class AllDifferentNode : public ViolationInvariantNode {
 private:
  propagation::VarId _intermediate{propagation::NULL_ID};

 public:
  explicit AllDifferentNode(std::vector<VarNodeId>&& variables, VarNodeId r);

  explicit AllDifferentNode(std::vector<VarNodeId>&& variables,
                            bool shouldHold);

  static std::vector<std::pair<std::string, size_t>> acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string, size_t>>{
        {"fzn_all_different_int", 1}, {"fzn_all_different_int_reif", 2}};
  }

  static std::unique_ptr<AllDifferentNode> fromModelConstraint(
      const fznparser::Constraint&, InvariantGraph&);

  bool prune(InvariantGraph&) override;

  void registerOutputVariables(InvariantGraph&, propagation::SolverBase& solver) override;

  void registerNode(InvariantGraph&, propagation::SolverBase& solver) override;
};
}  // namespace invariantgraph