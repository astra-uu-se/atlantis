#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/model.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"

namespace atlantis {
struct FznOutputVar;
struct FznOutputVarArray;
}  // namespace atlantis

namespace atlantis::invariantgraph {

class FznInvariantGraph : public InvariantGraph {
  std::unordered_set<std::string> _outputIdentifiers;
  std::vector<std::pair<std::string, VarNodeId>> _outputBoolVars;
  std::vector<std::pair<std::string, VarNodeId>> _outputIntVars;
  std::vector<InvariantGraphOutputVarArray> _outputBoolVarArrays;
  std::vector<InvariantGraphOutputVarArray> _outputIntVarArrays;

 public:
  explicit FznInvariantGraph(bool breakDynamicCycles = false);

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

  [[nodiscard]] std::vector<FznOutputVar> outputBoolVars(const SolverMapping&) const noexcept;
  [[nodiscard]] std::vector<FznOutputVar> outputIntVars(const SolverMapping&) const noexcept;
  [[nodiscard]] std::vector<FznOutputVarArray> outputBoolVarArrays(const SolverMapping&)
      const noexcept;
  [[nodiscard]] std::vector<FznOutputVarArray> outputIntVarArrays(const SolverMapping&)
      const noexcept;

  void build(const fznparser::Model&);

 private:
  void createNodes(const fznparser::Model&);

  bool makeInvariantNode(const fznparser::Constraint& constraint);
  bool makeViolationInvariantNode(const fznparser::Constraint& constraint);
};

}  // namespace atlantis::invariantgraph
