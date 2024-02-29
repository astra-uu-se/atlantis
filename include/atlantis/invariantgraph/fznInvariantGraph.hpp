#pragma once

#include <fznparser/model.hpp>
#include <fznparser/variables.hpp>
#include <unordered_set>
#include <utility>
#include <vector>

#include "atlantis/invariantgraph/implicitConstraintNodes/allDifferentImplicitNode.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/circuitImplicitNode.hpp"
#include "atlantis/invariantgraph/invariantGraphRoot.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElement2dNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/boolLinearNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/intDivNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/intMaxNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/intMinNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/intModNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPlusNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPowNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/intTimesNode.hpp"
#include "atlantis/invariantgraph/views/bool2IntNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/invariantgraph/views/intAbsNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolClauseNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolEqNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolLeNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolLtNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolOrNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolXorNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intEqNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intLeNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intLtNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intNeNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/setInNode.hpp"
#include "atlantis/utils/fznAst.hpp"
#include "atlantis/utils/fznOutput.hpp"
#include "atlantis/utils/variant.hpp"
#include "invariantGraph.hpp"

namespace atlantis::invariantgraph {

class FznInvariantGraph : public InvariantGraph {
 private:
  std::unordered_set<std::string> _outputIdentifiers;
  std::vector<std::pair<std::string, VarNodeId>> _outputBoolVars;
  std::vector<std::pair<std::string, VarNodeId>> _outputIntVars;
  std::vector<InvariantGraphOutputVarArray> _outputBoolVarArrays;
  std::vector<InvariantGraphOutputVarArray> _outputIntVarArrays;

 public:
  VarNodeId retrieveVarNode(const fznparser::BoolVar&);
  VarNodeId retrieveVarNode(std::reference_wrapper<const fznparser::BoolVar>);
  VarNodeId retrieveVarNode(const fznparser::BoolArg&);

  std::vector<VarNodeId> retrieveVarNodes(const fznparser::BoolVarArray&);

  VarNodeId retrieveVarNode(const fznparser::IntVar&);
  VarNodeId retrieveVarNode(const fznparser::IntArg&);
  VarNodeId retrieveVarNode(std::reference_wrapper<const fznparser::IntVar>);

  std::vector<VarNodeId> retrieveVarNodes(const fznparser::IntVarArray&);

  [[nodiscard]] std::vector<FznOutputVar> outputBoolVars() const noexcept;
  [[nodiscard]] std::vector<FznOutputVar> outputIntVars() const noexcept;
  [[nodiscard]] std::vector<FznOutputVarArray> outputBoolVarArrays()
      const noexcept;
  [[nodiscard]] std::vector<FznOutputVarArray> outputIntVarArrays()
      const noexcept;

  FznInvariantGraph();

  void build(const fznparser::Model&);

 private:
  void createNodes(const fznparser::Model&);

  bool makeInvariantNode(const fznparser::Constraint& constraint,
                         bool guessDefinedVar = false);
  bool makeImplicitConstraintNode(const fznparser::Constraint& constraint);
  bool makeViolationInvariantNode(const fznparser::Constraint& constraint);
};

}  // namespace atlantis::invariantgraph
