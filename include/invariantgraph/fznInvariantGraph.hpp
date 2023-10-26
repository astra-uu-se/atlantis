#pragma once

#include <fznparser/model.hpp>
#include <fznparser/variables.hpp>
#include <unordered_set>
#include <utility>
#include <vector>

#include "invariantGraph.hpp"
#include "utils/fznAst.hpp"
#include "utils/fznOutput.hpp"
#include "utils/variant.hpp"

namespace atlantis::invariantgraph {

class FznInvariantGraph : public InvariantGraph {
 private:
  std::unordered_set<std::string> _outputIdentifiers;
  std::vector<VarNodeId> _outputBoolVarNodeIds;
  std::vector<VarNodeId> _outputIntVarNodeIds;
  std::vector<InvariantGraphOutputVarArray> _outputBoolVarArrays;
  std::vector<InvariantGraphOutputVarArray> _outputIntVarArrays;

 public:
  VarNodeId createVarNode(const fznparser::BoolVar&);
  VarNodeId createVarNode(std::reference_wrapper<const fznparser::BoolVar>);
  VarNodeId createVarNode(const fznparser::BoolArg&);
  VarNodeId createVarNode(const fznparser::IntVar&);
  VarNodeId createVarNode(const fznparser::IntArg&);
  VarNodeId createVarNode(std::reference_wrapper<const fznparser::IntVar>);

  std::vector<VarNodeId> createVarNodes(const fznparser::BoolVarArray&);
  std::vector<VarNodeId> createVarNodes(const fznparser::IntVarArray&);

  [[nodiscard]] std::vector<FznOutputVar> outputBoolVars() const noexcept;
  [[nodiscard]] std::vector<FznOutputVar> outputIntVars() const noexcept;
  [[nodiscard]] std::vector<FznOutputVarArray> outputBoolVarArrays()
      const noexcept;
  [[nodiscard]] std::vector<FznOutputVarArray> outputIntVarArrays()
      const noexcept;

  FznInvariantGraph();

  void build(const fznparser::Model&);

 private:
  void initVarNodes(const fznparser::Model&);
  void createNodes(const fznparser::Model&);

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