#pragma once

#include <fznparser/model.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

#include "invariantGraph.hpp"
#include "utils/variant.hpp"

namespace atlantis::invariantgraph {
class InvariantGraphBuilder {
 private:
  fznparser::Model& _model;
  InvariantGraph _invariantGraph;

 public:
  InvariantGraphBuilder(fznparser::Model&);

  InvariantGraph build();

 private:
  void initVarNodes();
  void createNodes();

  void markOutputTo(const invariantgraph::InvariantNode& invNodeId,
                    std::unordered_set<std::string>& definedVars);

  std::unique_ptr<InvariantNode> makeInvariantNode(
      const fznparser::Constraint& constraint, bool guessDefinedVar = false);
  std::unique_ptr<ImplicitConstraintNode> makeImplicitConstraintNode(
      const fznparser::Constraint& constraint);
  std::unique_ptr<ViolationInvariantNode> makeViolationInvariantNode(
      const fznparser::Constraint& constraint);
};

}  // namespace atlantis::invariantgraph