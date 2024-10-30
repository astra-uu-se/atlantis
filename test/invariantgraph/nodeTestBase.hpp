#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <unordered_map>
#include <utility>
#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNode.hpp"
#include "atlantis/utils/variant.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class UnitInvariantNode : public InvariantNode {
 public:
  explicit UnitInvariantNode(IInvariantGraph& graph,
                             std::vector<VarNodeId>&& defVarNodes)
      : InvariantNode(graph, std::move(defVarNodes)) {}

  void registerOutputVars() override {
    for (const auto& varNodeId : outputVarNodeIds()) {
      makeSolverVar(varNodeId);
    }
  }

  void registerNode() override {}
};

enum class InvariantNodeAction : unsigned char {
  NONE = 0,
  SUBSUME = 1,
  REPLACE = 2,
  MAKE_IMPLICIT = 3
};

enum class ViolationInvariantType : unsigned char {
  CONSTANT_TRUE = 0,
  CONSTANT_FALSE = 1,
  REIFIED = 2
};

struct ParamData {
  InvariantNodeAction action;
  ViolationInvariantType violType;
  int data;
  explicit ParamData(InvariantNodeAction a, ViolationInvariantType vt, int d)
      : action(a), violType(vt), data(d) {}
  explicit ParamData(InvariantNodeAction a, ViolationInvariantType vt)
      : action(a), violType(vt), data(0) {}
  explicit ParamData(InvariantNodeAction a, int d)
      : action(a), violType(ViolationInvariantType::CONSTANT_TRUE), data(d) {}
  explicit ParamData(InvariantNodeAction a)
      : action(a), violType(ViolationInvariantType::CONSTANT_TRUE), data(0) {}

  explicit ParamData(ViolationInvariantType vt, int d)
      : action(InvariantNodeAction::NONE), violType(vt), data(d) {}
  explicit ParamData(ViolationInvariantType vt)
      : action(InvariantNodeAction::NONE), violType(vt) {}

  explicit ParamData(int d)
      : action(InvariantNodeAction::NONE),
        violType(ViolationInvariantType::CONSTANT_TRUE),
        data(d) {}

  explicit ParamData()
      : action(InvariantNodeAction::NONE),
        violType(ViolationInvariantType::CONSTANT_TRUE),
        data(0) {}
};

template <class InvNode>
class NodeTestBase : public ::testing::TestWithParam<ParamData> {
 protected:
  ParamData _paramData{0};

  bool shouldBeSubsumed() const {
    return _paramData.action == InvariantNodeAction::SUBSUME;
  }
  bool shouldBeReplaced() const {
    return _paramData.action == InvariantNodeAction::REPLACE;
  }
  bool shouldBeMadeImplicit() const {
    return _paramData.action == InvariantNodeAction::MAKE_IMPLICIT;
  }
  bool shouldHold() const {
    return _paramData.violType == ViolationInvariantType::CONSTANT_TRUE;
  }
  bool shouldFail() const {
    return _paramData.violType == ViolationInvariantType::CONSTANT_FALSE;
  }
  bool isReified() const {
    return _paramData.violType == ViolationInvariantType::REIFIED;
  }

  std::shared_ptr<propagation::Solver> _solver{nullptr};
  std::shared_ptr<InvariantGraph> _invariantGraph{nullptr};

  InvariantNodeId _invNodeId = InvariantNodeId{NULL_NODE_ID};

  void SetUp() override {
    _solver = std::make_unique<propagation::Solver>();
    _invariantGraph = std::make_unique<InvariantGraph>(*_solver);
    _paramData = GetParam();
  }

  template <typename... Args>
  void createInvariantNode(Args&&... args) {
    EXPECT_EQ(_invNodeId, NULL_NODE_ID);
    _invNodeId = _invariantGraph->addInvariantNode(
        std::make_unique<InvNode>(std::forward<Args>(args)...));
  }

  template <typename... Args>
  void createImplicitConstraintNode(Args&&... args) {
    EXPECT_EQ(_invNodeId, NULL_NODE_ID);
    _invNodeId = _invariantGraph->addImplicitConstraintNode(
        std::make_unique<InvNode>(std::forward<Args>(args)...));
  }

  InvNode& invNode() {
    EXPECT_NE(_invNodeId, NULL_NODE_ID);
    if (_invNodeId.isInvariant()) {
      return dynamic_cast<InvNode&>(_invariantGraph->invariantNode(_invNodeId));
    } else {
      return dynamic_cast<InvNode&>(
          _invariantGraph->implicitConstraintNode(_invNodeId));
    }
  }

  VarNodeId retrieveIntVarNode(Int lb, Int ub, const std::string& identifier) {
    return _invariantGraph->retrieveIntVarNode(SearchDomain(lb, ub),
                                               identifier);
  }

  VarNodeId retrieveIntVarNode(std::vector<Int>&& vals,
                               const std::string& identifier) {
    assert(!vals.empty());
    return _invariantGraph->retrieveIntVarNode(SearchDomain(std::move(vals)),
                                               identifier);
  }

  VarNodeId retrieveIntVarNode(Int val) {
    return _invariantGraph->retrieveIntVarNode(val);
  }

  VarNodeId retrieveBoolVarNode(const std::string& identifier) {
    return _invariantGraph->retrieveBoolVarNode(identifier);
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

  propagation::VarViewId varId(const std::string& identifier) {
    return varNode(identifier).varId();
  }

  propagation::VarViewId varId(VarNodeId varNodeId) {
    return varNode(varNodeId).varId();
  }

  std::string identifier(VarNodeId varNodeId) {
    return varNode(varNodeId).identifier();
  }

  void addInputVarsToSolver() {
    EXPECT_EQ(_solver->numVars(), 0);
    std::unordered_set<size_t> visited;
    visited.reserve(invNode().staticInputVarNodeIds().size() +
                    invNode().dynamicInputVarNodeIds().size());
    for (const VarNodeId varNodeId : invNode().staticInputVarNodeIds()) {
      if (visited.contains(size_t(varNodeId))) {
        EXPECT_NE(varId(varNodeId), propagation::NULL_ID);
      } else {
        if (!varNode(varNodeId).isFixed()) {
          EXPECT_EQ(varId(varNodeId), propagation::NULL_ID);
        }
        visited.emplace(size_t(varNodeId));
      }
      if (varId(varNodeId) == propagation::NULL_ID) {
        const auto& [lb, ub] = varNode(varNodeId).bounds();
        varNode(varNodeId).setVarId(_solver->makeIntVar(lb, lb, ub));
      }
      EXPECT_NE(varId(varNodeId), propagation::NULL_ID);
    }
    for (const VarNodeId varNodeId : invNode().dynamicInputVarNodeIds()) {
      if (visited.contains(size_t(varNodeId))) {
        EXPECT_NE(varId(varNodeId), propagation::NULL_ID);
      } else {
        if (!varNode(varNodeId).isFixed()) {
          EXPECT_EQ(varId(varNodeId), propagation::NULL_ID);
        }
        visited.emplace(size_t(varNodeId));
      }
      if (varId(varNodeId) == propagation::NULL_ID) {
        const auto& [lb, ub] = varNode(varNodeId).bounds();
        varNode(varNodeId).setVarId(_solver->makeIntVar(lb, lb, ub));
      }
      EXPECT_NE(varId(varNodeId), propagation::NULL_ID);
    }
    expectInputsRegistered(invNode());
  }

  [[nodiscard]] inline propagation::VarViewId solverVarId(
      const VarNodeId varNodeId) const {
    return _invariantGraph->varNode(varNodeId).varId();
  }

  void expectInputsRegistered(const InvariantNode& invNode) {
    std::vector<bool> registered(_solver->numVars(), false);
    for (const auto& varNodeId : invNode.staticInputVarNodeIds()) {
      EXPECT_TRUE(solverVarId(varNodeId).isVar());
      EXPECT_NE(solverVarId(varNodeId), propagation::NULL_ID);
      if (!varNode(varNodeId).isFixed()) {
        EXPECT_FALSE(registered.at(size_t(solverVarId(varNodeId))));
      }
      registered.at(size_t(solverVarId(varNodeId))) = true;
    }
    for (const auto& varNodeId : invNode.dynamicInputVarNodeIds()) {
      EXPECT_TRUE(solverVarId(varNodeId).isVar());
      EXPECT_NE(solverVarId(varNodeId), propagation::NULL_ID);
      if (!varNode(varNodeId).isFixed()) {
        EXPECT_FALSE(registered.at(size_t(solverVarId(varNodeId))));
      }
      registered.at(size_t(solverVarId(varNodeId))) = true;
    }
    for (const bool r : registered) {
      EXPECT_TRUE(r);
    }
  }

  void expectInputTo(const InvariantNode& invNode) {
    for (const auto& varNodeId : invNode.staticInputVarNodeIds()) {
      bool found = false;
      for (const auto& invNodeId :
           _invariantGraph->varNode(varNodeId).staticInputTo()) {
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
           _invariantGraph->varNode(varNodeId).dynamicInputTo()) {
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
      const std::vector<propagation::VarViewId>& inputVars) {
    std::vector<Int> inputVals;
    inputVals.reserve(inputVars.size());
    for (const propagation::VarViewId& varId : inputVars) {
      inputVals.emplace_back(_solver->lowerBound(varId));
    }
    return inputVals;
  }

  Int increaseNextVal(const std::vector<propagation::VarViewId>& inputVars,
                      std::vector<Int>& inputVals) {
    EXPECT_EQ(inputVars.size(), inputVals.size());
    for (Int i = static_cast<Int>(inputVals.size()) - 1; i >= 0; --i) {
      if (inputVars.at(i) == propagation::NULL_ID) {
        continue;
      }
      if (inputVals.at(i) < _solver->upperBound(inputVars.at(i))) {
        ++inputVals.at(i);
        return i;
      }
      inputVals.at(i) = _solver->lowerBound(inputVars.at(i));
    }
    return -1;
  }

  void setVarVals(const std::vector<propagation::VarViewId>& inputVars,
                  const std::vector<Int>& vals) {
    EXPECT_EQ(inputVars.size(), vals.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      if (inputVars.at(i) != propagation::NULL_ID) {
        _solver->setValue(inputVars.at(i), vals.at(i));
      }
    }
  }

  void expectVarVals(const std::vector<propagation::VarViewId>& inputVars,
                     const std::vector<Int>& vals) {
    EXPECT_EQ(inputVars.size(), vals.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      if (inputVars.at(i) != propagation::NULL_ID) {
        EXPECT_EQ(_solver->currentValue(inputVars.at(i)), vals.at(i));
      }
    }
  }

  void updateOutputVals(const std::vector<propagation::VarViewId>& outputVars,
                        std::vector<Int>& outputVals) {
    EXPECT_EQ(outputVars.size(), outputVals.size());
    for (size_t i = 0; i < outputVars.size(); ++i) {
      outputVals.at(i) = _solver->currentValue(outputVars.at(i));
    }
  }
};

}  // namespace atlantis::testing
