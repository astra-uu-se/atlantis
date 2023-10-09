#pragma once

#include <gtest/gtest.h>

#include <unordered_map>
#include <utility>
#include <vector>

#include "core/propagationEngine.hpp"
#include "fznparser/model.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/types.hpp"
#include "utils/variant.hpp"

template <typename InvNode>
class NodeTestBase : public testing::Test {
 protected:
  std::unique_ptr<fznparser::Model> _model;
  std::unique_ptr<invariantgraph::InvariantGraph> _invariantGraph;

  invariantgraph::InvariantNodeId _invNodeId =
      invariantgraph::InvariantNodeId(invariantgraph::NULL_NODE_ID);

  void SetUp() override {
    _model = std::make_unique<fznparser::Model>();
    _invariantGraph = std::make_unique<invariantgraph::InvariantGraph>();
  }

  InvNode& invNode() {
    EXPECT_NE(_invNodeId, invariantgraph::NULL_NODE_ID);
    if (_invNodeId.type == invariantgraph::InvariantNodeId::Type::INVARIANT) {
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

  invariantgraph::VarNodeId createIntVar(int64_t lowerBound, int64_t upperBound,
                                         std::string identifier) {
    EXPECT_FALSE(_model->hasVariable(identifier));
    const fznparser::IntVar& var =
        std::get<fznparser::IntVar>(_model->addVariable(
            std::move(fznparser::IntVar(lowerBound, upperBound, identifier))));
    return _invariantGraph->createVarNode(var);
  }

  invariantgraph::VarNodeId createBoolVar(std::string identifier) {
    EXPECT_FALSE(_model->hasVariable(identifier));
    const fznparser::BoolVar& var = std::get<fznparser::BoolVar>(
        _model->addVariable(std::move(fznparser::BoolVar(identifier))));
    return _invariantGraph->createVarNode(var);
  }

  invariantgraph::VarNode& varNode(const std::string& identifier) {
    return _invariantGraph->varNode(identifier);
  }

  invariantgraph::VarNode& varNode(invariantgraph::VarNodeId varNodeId) {
    return _invariantGraph->varNode(varNodeId);
  }

  const fznparser::IntVar& intVar(const std::string& identifier) {
    return std::get<fznparser::IntVar>(_model->variable(identifier));
  }

  const fznparser::IntVar& intVar(invariantgraph::VarNodeId varNodeId) {
    return std::get<fznparser::IntVar>(
        _model->variable(varNode(varNodeId).identifier()));
  }

  const fznparser::BoolVar& boolVar(const std::string& identifier) {
    return std::get<fznparser::BoolVar>(_model->variable(identifier));
  }

  const fznparser::BoolVar& boolVar(invariantgraph::VarNodeId varNodeId) {
    return std::get<fznparser::BoolVar>(
        _model->variable(varNode(varNodeId).identifier()));
  }

  VarId varId(const std::string& identifier) {
    return varNode(identifier).varId();
  }

  VarId varId(invariantgraph::VarNodeId varNodeId) {
    return varNode(varNodeId).varId();
  }

  std::string identifier(invariantgraph::VarNodeId varNodeId) {
    return varNode(varNodeId).identifier();
  }

  fznparser::Annotation definesVarAnnotation(std::string identifier) {
    return fznparser::Annotation(
        "defines_var",
        std::vector<std::vector<fznparser::AnnotationExpression>>{
            std::vector<fznparser::AnnotationExpression>{
                fznparser::Annotation(identifier)}});
  }

  void addInputVarsToEngine(PropagationEngine& engine) {
    EXPECT_EQ(engine.numVariables(), 0);
    for (const auto& varNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_EQ(varId(varNodeId), NULL_ID);
      const auto& [lb, ub] = varNode(varNodeId).bounds();
      varNode(varNodeId).setVarId(engine.makeIntVar(lb, lb, ub));
      EXPECT_NE(varId(varNodeId), NULL_ID);
    }
    for (const auto& varNodeId : invNode().dynamicInputVarNodeIds()) {
      EXPECT_EQ(varId(varNodeId), NULL_ID);
      const auto& [lb, ub] = varNode(varNodeId).bounds();
      varNode(varNodeId).setVarId(engine.makeIntVar(lb, lb, ub));
      EXPECT_NE(varId(varNodeId), NULL_ID);
    }
    EXPECT_EQ(engine.numVariables(),
              invNode().staticInputVarNodeIds().size() +
                  invNode().dynamicInputVarNodeIds().size());
    expectInputsRegistered(invNode(), engine);
  }

  [[nodiscard]] inline VarId engineVarId(
      const invariantgraph::VarNodeId& varNodeId) const {
    return _invariantGraph->varNode(varNodeId).varId();
  }

  void expectInputTo(const invariantgraph::InvariantNode& invNode) {
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

  void expectOutputOf(const invariantgraph::InvariantNode& invNode) {
    for (const auto& varNodeId : invNode.outputVarNodeIds()) {
      EXPECT_EQ(_invariantGraph->varNode(varNodeId).outputOf(), invNode.id());
    }
  }

  void expectInputsRegistered(const invariantgraph::InvariantNode& invNode,
                              const PropagationEngine& engine) {
    EXPECT_EQ(engine.numVariables(),
              invNode.staticInputVarNodeIds().size() +
                  invNode.dynamicInputVarNodeIds().size());
    std::vector<bool> registered(engine.numVariables(), false);
    for (const auto& varNodeId : invNode.staticInputVarNodeIds()) {
      EXPECT_NE(engineVarId(varNodeId), NULL_ID);
      EXPECT_FALSE(registered.at(engineVarId(varNodeId) - 1));
      registered.at(engineVarId(varNodeId) - 1) = true;
    }
    for (const auto& varNodeId : invNode.dynamicInputVarNodeIds()) {
      EXPECT_NE(engineVarId(varNodeId), NULL_ID);
      EXPECT_FALSE(registered.at(engineVarId(varNodeId) - 1));
      registered.at(engineVarId(varNodeId) - 1) = true;
    }
    for (size_t i = 0; i < registered.size(); ++i) {
      EXPECT_TRUE(registered.at(i));
    }
  }
};

enum class ConstraintType : unsigned char {
  NORMAL = 0,
  REIFIED = 1,
  CONSTANT_FALSE = 2,
  CONSTANT_TRUE = 3
};