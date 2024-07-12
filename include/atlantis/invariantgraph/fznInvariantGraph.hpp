#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/model.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/utils/fznOutput.hpp"

namespace atlantis::invariantgraph {

class FznInvariantGraph : public InvariantGraph {
 private:
  std::unordered_set<std::string> _outputIdentifiers;
  std::vector<std::pair<std::string, VarNodeId>> _outputBoolVars;
  std::vector<std::pair<std::string, VarNodeId>> _outputIntVars;
  std::vector<InvariantGraphOutputVarArray> _outputBoolVarArrays;
  std::vector<InvariantGraphOutputVarArray> _outputIntVarArrays;

 public:
  FznInvariantGraph(bool breakDynamicCycles = false);

  VarNodeId retrieveVarNode(const fznparser::BoolVar&);
  VarNodeId retrieveVarNode(const std::shared_ptr<const fznparser::BoolVar>&);
  VarNodeId retrieveVarNode(const fznparser::BoolArg&);

  std::vector<VarNodeId> retrieveVarNodes(
      const std::shared_ptr<fznparser::BoolVarArray>&);

  VarNodeId retrieveVarNode(const fznparser::IntVar&);
  VarNodeId retrieveVarNode(const fznparser::IntArg&);
  VarNodeId retrieveVarNode(const std::shared_ptr<const fznparser::IntVar>&);

  std::vector<VarNodeId> retrieveVarNodes(
      const std::shared_ptr<fznparser::IntVarArray>&);

  [[nodiscard]] std::vector<FznOutputVar> outputBoolVars() const noexcept;
  [[nodiscard]] std::vector<FznOutputVar> outputIntVars() const noexcept;
  [[nodiscard]] std::vector<FznOutputVarArray> outputBoolVarArrays()
      const noexcept;
  [[nodiscard]] std::vector<FznOutputVarArray> outputIntVarArrays()
      const noexcept;

  void build(const fznparser::Model&);

 private:
  void createNodes(const fznparser::Model&);

  bool makeInvariantNode(const fznparser::Constraint& constraint,
                         bool guessDefinedVar = false);
  bool makeImplicitConstraintNode(const fznparser::Constraint& constraint);
  bool makeViolationInvariantNode(const fznparser::Constraint& constraint);
};

}  // namespace atlantis::invariantgraph
