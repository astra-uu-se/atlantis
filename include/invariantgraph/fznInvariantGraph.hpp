#pragma once

#include <fznparser/model.hpp>
#include <fznparser/variables.hpp>
#include <unordered_set>
#include <utility>
#include <vector>

#include "invariantGraph.hpp"
#include "invariantgraph/implicitConstraints/allDifferentImplicitNode.hpp"
#include "invariantgraph/implicitConstraints/circuitImplicitNode.hpp"
#include "invariantgraph/invariantGraphRoot.hpp"
#include "invariantgraph/invariantNodes/arrayBoolElement2dNode.hpp"
#include "invariantgraph/invariantNodes/arrayBoolElementNode.hpp"
#include "invariantgraph/invariantNodes/arrayIntElement2dNode.hpp"
#include "invariantgraph/invariantNodes/arrayIntElementNode.hpp"
#include "invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"
#include "invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"
#include "invariantgraph/invariantNodes/arrayVarBoolElement2dNode.hpp"
#include "invariantgraph/invariantNodes/arrayVarBoolElementNode.hpp"
#include "invariantgraph/invariantNodes/arrayVarIntElement2dNode.hpp"
#include "invariantgraph/invariantNodes/arrayVarIntElementNode.hpp"
#include "invariantgraph/invariantNodes/boolLinearNode.hpp"
#include "invariantgraph/invariantNodes/intDivNode.hpp"
#include "invariantgraph/invariantNodes/intLinearNode.hpp"
#include "invariantgraph/invariantNodes/intMaxNode.hpp"
#include "invariantgraph/invariantNodes/intMinNode.hpp"
#include "invariantgraph/invariantNodes/intModNode.hpp"
#include "invariantgraph/invariantNodes/intPlusNode.hpp"
#include "invariantgraph/invariantNodes/intPowNode.hpp"
#include "invariantgraph/invariantNodes/intTimesNode.hpp"
#include "invariantgraph/views/bool2IntNode.hpp"
#include "invariantgraph/views/boolNotNode.hpp"
#include "invariantgraph/views/intAbsNode.hpp"
#include "invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolAndNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolClauseNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolEqNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolLeNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolLinEqNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolLinLeNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolLtNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolOrNode.hpp"
#include "invariantgraph/violationInvariantNodes/boolXorNode.hpp"
#include "invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "invariantgraph/violationInvariantNodes/intEqNode.hpp"
#include "invariantgraph/violationInvariantNodes/intLeNode.hpp"
#include "invariantgraph/violationInvariantNodes/intLinEqNode.hpp"
#include "invariantgraph/violationInvariantNodes/intLinLeNode.hpp"
#include "invariantgraph/violationInvariantNodes/intLinNeNode.hpp"
#include "invariantgraph/violationInvariantNodes/intLtNode.hpp"
#include "invariantgraph/violationInvariantNodes/intNeNode.hpp"
#include "invariantgraph/violationInvariantNodes/setInNode.hpp"
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
  VarNodeId createVarNodeFromFzn(const fznparser::BoolVar&, bool isDefinedVar);
  VarNodeId createVarNodeFromFzn(
      std::reference_wrapper<const fznparser::BoolVar>, bool isDefinedVar);
  VarNodeId createVarNodeFromFzn(const fznparser::BoolArg&, bool isDefinedVar);
  VarNodeId createVarNodeFromFzn(const fznparser::IntVar&, bool isDefinedVar);
  VarNodeId createVarNodeFromFzn(const fznparser::IntArg&, bool isDefinedVar);
  VarNodeId createVarNodeFromFzn(
      std::reference_wrapper<const fznparser::IntVar>, bool isDefinedVar);

  std::vector<VarNodeId> createVarNodes(const fznparser::BoolVarArray&,
                                        bool areDefinedVars);
  std::vector<VarNodeId> createVarNodes(const fznparser::IntVarArray&,
                                        bool areDefinedVars);

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