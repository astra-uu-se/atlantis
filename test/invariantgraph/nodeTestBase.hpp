#pragma once

#include <gtest/gtest.h>

#include <unordered_map>
#include <utility>
#include <vector>

#include "fznparser/model.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/types.hpp"
#include "propagation/solver.hpp"
#include "utils/variant.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

template <typename InvNode>
class NodeTestBase : public ::testing::Test {
 protected:
  std::unique_ptr<fznparser::Model> _model;
  std::unique_ptr<InvariantGraph> _invariantGraph;

  InvariantNodeId _invNodeId = InvariantNodeId(NULL_NODE_ID);

  void SetUp() override {
    _model = std::make_unique<fznparser::Model>();
    _invariantGraph = std::make_unique<InvariantGraph>();
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

  void makeInvNode(const fznparser::Constraint& constraint) {
    _invNodeId = _invariantGraph->addInvariantNode(
        std::move(InvNode::fromModelConstraint(constraint, *_invariantGraph)));
  }

  void makeImplicitNode(const fznparser::Constraint& constraint) {
    _invNodeId = _invariantGraph->addImplicitConstraintNode(
        std::move(InvNode::fromModelConstraint(constraint, *_invariantGraph)));
  }

  template <typename OtherInvNode>
  void makeOtherInvNode(const fznparser::Constraint& constraint) {
    _invNodeId = _invariantGraph->addInvariantNode(std::move(
        OtherInvNode::fromModelConstraint(constraint, *_invariantGraph)));
  }

  VarNodeId createIntVar(int64_t lowerBound, int64_t upperBound,
                         std::string identifier) {
    EXPECT_FALSE(_model->hasVar(identifier));
    const fznparser::IntVar& var = std::get<fznparser::IntVar>(_model->addVar(
        std::move(fznparser::IntVar(lowerBound, upperBound, identifier))));
    return _invariantGraph->createVarNode(var);
  }

  VarNodeId createBoolVar(std::string identifier) {
    EXPECT_FALSE(_model->hasVar(identifier));
    const fznparser::BoolVar& var = std::get<fznparser::BoolVar>(
        _model->addVar(std::move(fznparser::BoolVar(identifier))));
    return _invariantGraph->createVarNode(var);
  }

  VarNode& varNode(const std::string& identifier) {
    return _invariantGraph->varNode(identifier);
  }

  VarNode& varNode(VarNodeId varNodeId) {
    return _invariantGraph->varNode(varNodeId);
  }

  const fznparser::IntVar& intVar(const std::string& identifier) {
    return std::get<fznparser::IntVar>(_model->var(identifier));
  }

  const fznparser::IntVar& intVar(VarNodeId varNodeId) {
    return std::get<fznparser::IntVar>(
        _model->var(varNode(varNodeId).identifier()));
  }

  const fznparser::BoolVar& boolVar(const std::string& identifier) {
    return std::get<fznparser::BoolVar>(_model->var(identifier));
  }

  const fznparser::BoolVar& boolVar(VarNodeId varNodeId) {
    return std::get<fznparser::BoolVar>(
        _model->var(varNode(varNodeId).identifier()));
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

  fznparser::Annotation definesVarAnnotation(std::string identifier) {
    return fznparser::Annotation(
        "defines_var",
        std::vector<std::vector<fznparser::AnnotationExpression>>{
            std::vector<fznparser::AnnotationExpression>{
                fznparser::Annotation(identifier)}});
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
};

enum class ViolationInvariantType : unsigned char {
  NORMAL = 0,
  REIFIED = 1,
  CONSTANT_FALSE = 2,
  CONSTANT_TRUE = 3
};
}  // namespace atlantis::testing