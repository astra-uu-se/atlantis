#pragma once

#include <gtest/gtest.h>

#include <unordered_map>
#include <utility>
#include <vector>

#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/types.hpp"
#include "propagation/solver.hpp"
#include "utils/variant.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

template <class InvNode>
class NodeTestBase : public ::testing::Test {
 protected:
  std::unique_ptr<InvariantGraph> _invariantGraph;

  InvariantNodeId _invNodeId = InvariantNodeId(NULL_NODE_ID);

  void SetUp() override {
    _invariantGraph = std::make_unique<InvariantGraph>();
  }

  template <typename... Args>
  void createInvariantNode(Args&&... args) {
    EXPECT_TRUE(_invNodeId == NULL_NODE_ID);
    _invNodeId = _invariantGraph->addInvariantNode(
        std::make_unique<InvNode>(std::forward<Args>(args)...));
  }

  template <typename... Args>
  void createImplicitConstraintNode(Args&&... args) {
    EXPECT_TRUE(_invNodeId == NULL_NODE_ID);
    _invNodeId = _invariantGraph->addImplicitConstraintNode(
        std::make_unique<InvNode>(std::forward<Args>(args)...));
  }

  InvNode& invNode() {
    EXPECT_NE(_invNodeId, NULL_NODE_ID);
    if (_invNodeId.type == InvariantNodeId::Type::INVARIANT) {
      return dynamic_cast<InvNode&>(_invariantGraph->invariantNode(_invNodeId));
    } else {
      return dynamic_cast<InvNode&>(
          _invariantGraph->implicitConstraintNode(_invNodeId));
    }
  }

  VarNodeId createIntVarNode(Int lb, Int ub, const std::string& identifier,
                             bool isDefinedVar = false) {
    return _invariantGraph->createVarNode(SearchDomain{lb, ub}, true,
                                          identifier, isDefinedVar);
  }

  VarNodeId createIntVarNode(Int val, bool isDefinedVar = false) {
    return _invariantGraph->createVarNode(val, isDefinedVar);
  }

  VarNodeId createBoolVarNode(const std::string& identifier,
                              bool isDefinedVar = false) {
    return _invariantGraph->createVarNode(SearchDomain{0, 1}, false, identifier,
                                          isDefinedVar);
  }

  VarNodeId varNodeId(const std::string& identifier) {
    return _invariantGraph->varNodeId(identifier);
  }

  VarNode& varNode(const std::string& identifier) {
    return _invariantGraph->varNode(identifier);
  }

  VarNode& varNode(VarNodeId varNodeId) {
    return _invariantGraph->varNode(varNodeId);
  }

  propagation::VarId varId(const std::string& identifier) {
    return varNode(identifier).varId();
  }

  propagation::VarId varId(VarNodeId varNodeId) {
    return varNode(varNodeId).varId();
  }

  std::string identifier(VarNodeId varNodeId) {
    return varNode(varNodeId).identifier();
  }

  void addInputVarsToSolver(propagation::Solver& solver) {
    EXPECT_EQ(solver.numVars(), 0);
    for (const auto& varNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_EQ(varId(varNodeId), propagation::NULL_ID);
      const auto& [lb, ub] = varNode(varNodeId).bounds();
      varNode(varNodeId).setVarId(solver.makeIntVar(lb, lb, ub));
      EXPECT_NE(varId(varNodeId), propagation::NULL_ID);
    }
    for (const auto& varNodeId : invNode().dynamicInputVarNodeIds()) {
      EXPECT_EQ(varId(varNodeId), propagation::NULL_ID);
      const auto& [lb, ub] = varNode(varNodeId).bounds();
      varNode(varNodeId).setVarId(solver.makeIntVar(lb, lb, ub));
      EXPECT_NE(varId(varNodeId), propagation::NULL_ID);
    }
    EXPECT_EQ(solver.numVars(), invNode().staticInputVarNodeIds().size() +
                                    invNode().dynamicInputVarNodeIds().size());
    expectInputsRegistered(invNode(), solver);
  }

  [[nodiscard]] inline propagation::VarId solverVarId(
      const VarNodeId& varNodeId) const {
    return _invariantGraph->varNode(varNodeId).varId();
  }

  void expectInputsRegistered(const InvariantNode& invNode,
                              const propagation::Solver& solver) {
    EXPECT_EQ(solver.numVars(), invNode.staticInputVarNodeIds().size() +
                                    invNode.dynamicInputVarNodeIds().size());
    std::vector<bool> registered(solver.numVars(), false);
    for (const auto& varNodeId : invNode.staticInputVarNodeIds()) {
      EXPECT_NE(solverVarId(varNodeId), propagation::NULL_ID);
      EXPECT_FALSE(registered.at(solverVarId(varNodeId) - 1));
      registered.at(solverVarId(varNodeId) - 1) = true;
    }
    for (const auto& varNodeId : invNode.dynamicInputVarNodeIds()) {
      EXPECT_NE(solverVarId(varNodeId), propagation::NULL_ID);
      EXPECT_FALSE(registered.at(solverVarId(varNodeId) - 1));
      registered.at(solverVarId(varNodeId) - 1) = true;
    }
    for (size_t i = 0; i < registered.size(); ++i) {
      EXPECT_TRUE(registered.at(i));
    }
  }

  void expectInputTo(const InvariantNode& invNode) {
    for (const auto& varNodeId : invNode.staticInputVarNodeIds()) {
      bool found = false;
      for (const auto& invNodeId :
           _invariantGraph->varNode(varNodeId).inputTo()) {
        if (invNodeId == invNode.id()) {
          found = true;
          break;
        }
      }
      EXPECT_TRUE(found);
    }
    for (const auto& varNodeId : invNode.dynamicInputVarNodeIds()) {
      bool found = false;
      for (const auto& invNodeId :
           _invariantGraph->varNode(varNodeId).inputTo()) {
        if (invNodeId == invNode.id()) {
          found = true;
          break;
        }
      }
      EXPECT_TRUE(found);
    }
  }

  void expectOutputOf(const InvariantNode& invNode) {
    for (const auto& varNodeId : invNode.outputVarNodeIds()) {
      EXPECT_EQ(_invariantGraph->varNode(varNodeId).outputOf(), invNode.id());
    }
  }

  std::vector<Int> makeInputVals(
      const propagation::Solver& solver,
      const std::vector<propagation::VarId>& inputVars) {
    std::vector<Int> inputVals;
    inputVals.reserve(inputVars.size());
    for (const propagation::VarId varId : inputVars) {
      inputVals.emplace_back(solver.lowerBound(varId));
    }
    return inputVals;
  }

  bool increaseNextVal(const propagation::Solver& solver,
                       const std::vector<propagation::VarId>& inputVars,
                       std::vector<Int>& inputVals) {
    EXPECT_EQ(inputVars.size(), inputVals.size());
    for (Int i = inputVals.size() - 1; i >= 0; --i) {
      if (inputVals.at(i) < solver.upperBound(inputVars.at(i))) {
        ++inputVals.at(i);
        return true;
      }
      inputVals.at(i) = solver.lowerBound(inputVars.at(i));
    }
    return false;
  }

  void setVarVals(propagation::Solver& solver,
                  const std::vector<propagation::VarId>& inputVars,
                  const std::vector<Int>& vals) {
    EXPECT_EQ(inputVars.size(), vals.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      solver.setValue(inputVars.at(i), vals.at(i));
    }
  }

  void updateOutputVals(propagation::Solver& solver,
                        const std::vector<propagation::VarId>& outputVars,
                        std::vector<Int>& outputVals) {
    EXPECT_EQ(outputVars.size(), outputVals.size());
    for (size_t i = 0; i < outputVars.size(); ++i) {
      outputVals.at(i) = solver.currentValue(outputVars.at(i));
    }
  }
};

enum class ViolationInvariantType : unsigned char {
  REIFIED = 0,
  CONSTANT_FALSE = 1,
  CONSTANT_TRUE = 2
};
}  // namespace atlantis::testing