#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/model.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/utils/fznOutput.hpp"

namespace atlantis {
struct FznOutputVar;
struct FznOutputVarArray;
}  // namespace atlantis

namespace atlantis::invariantgraph {

class FznInvariantGraph : public InvariantGraph {
  std::unordered_set<std::string> _outputIdentifiers;
  std::vector<std::pair<std::string, std::shared_ptr<VarNode>>> _outputBoolVars;
  std::vector<std::pair<std::string, std::shared_ptr<VarNode>>> _outputIntVars;
  std::vector<InvariantGraphOutputVarArray> _outputBoolVarArrays;
  std::vector<InvariantGraphOutputVarArray> _outputIntVarArrays;

 public:
  explicit FznInvariantGraph(bool breakDynamicCycles = false);

  VarNode& retrieveVarNode(const fznparser::BoolVar&);
  VarNode& retrieveVarNode(const std::shared_ptr<const fznparser::BoolVar>&);
  VarNode& retrieveVarNode(const fznparser::BoolArg&);

  std::vector<std::shared_ptr<VarNode>> retrieveVarNodes(
      const std::shared_ptr<fznparser::BoolVarArray>&);

  VarNode& retrieveVarNode(const fznparser::IntVar&);
  VarNode& retrieveVarNode(const fznparser::IntArg&);
  VarNode& retrieveVarNode(const std::shared_ptr<const fznparser::IntVar>&);

  std::vector<std::shared_ptr<VarNode>> retrieveVarNodes(
      const std::shared_ptr<fznparser::IntVarArray>&);

  [[nodiscard]] std::vector<FznOutputVar> outputBoolVars() const noexcept;
  [[nodiscard]] std::vector<FznOutputVar> outputIntVars() const noexcept;
  [[nodiscard]] std::vector<FznOutputVarArray> outputBoolVarArrays()
      const noexcept;
  [[nodiscard]] std::vector<FznOutputVarArray> outputIntVarArrays()
      const noexcept;
  [[nodiscard]] FznOutput generateFznOutput() const;

  void build(const fznparser::Model&);

 private:
  void createNodes(const fznparser::Model&);

  bool makeInvariantNode(const fznparser::Constraint& constraint);
  bool makeViolationInvariantNode(const fznparser::Constraint& constraint);
};

}  // namespace atlantis::invariantgraph
