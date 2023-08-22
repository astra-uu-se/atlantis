#pragma once

#include <gtest/gtest.h>

#include <unordered_map>
#include <utility>
#include <vector>

#include "core/propagationEngine.hpp"
#include "fznparser/model.hpp"
#include "invariantgraph/invariantGraph.hpp"
#include "invariantgraph/varNode.hpp"
#include "utils/variant.hpp"

class NodeTestBase : public testing::Test {
 protected:
  std::unique_ptr<fznparser::Model> _model;
  std::unique_ptr<invariantgraph::InvariantGraph> _invariantGraph;
  std::unique_ptr<
      std::unordered_map<std::string_view, invariantgraph::VarNodeId>>
      _nodeMap;

  void SetUp() override {
    _model = std::make_unique<fznparser::Model>();
    _nodeMap = std::make_unique<
        std::unordered_map<std::string_view, invariantgraph::VarNodeId>>();
    _invariantGraph = std::make_unique<invariantgraph::InvariantGraph>();
  }

  template <typename Node>
  inline std::unique_ptr<Node> makeNode(
      const fznparser::Constraint& constraint) {
    return Node::fromModelConstraint(*_model, constraint, *_invariantGraph);
  }

  fznparser::IntVar intVar(int64_t lowerBound, int64_t upperBound,
                           std::string_view identifier) {
    EXPECT_FALSE(_model->hasVariable(identifier));
    const fznparser::IntVar& var =
        std::get<fznparser::IntVar>(_model->createVarNode(
            std::move(fznparser::IntVar(lowerBound, upperBound, identifier))));
    EXPECT_FALSE(_nodeMap->contains(identifier));
    _nodeMap->emplace(identifier, _invariantGraph->createVarNode(var));
    return std::make_unique<fznparser::IntVar>(var);
  }

  fznparser::BoolVar boolVar(std::string_view identifier) {
    EXPECT_FALSE(_model->hasVariable(identifier));
    const fznparser::BoolVar& var = std::get<fznparser::BoolVar>(
        _model->createVarNode(std::move(fznparser::BoolVar(identifier))));
    EXPECT_FALSE(_nodeMap->contains(identifier));
    _nodeMap->emplace(identifier, _invariantGraph->createVarNode(var));
    return std::make_unique<fznparser::BoolVar>(var);
  }

  fznparser::Annotation definesVarAnnotation(std::string_view identifier) {
    return fznparser::Annotation(
        "defines_var",
        std::vector<std::vector<fznparser::AnnotationExpression>>{
            std::vector<fznparser::AnnotationExpression>{
                fznparser::Annotation(identifier)}});
  }

  inline void addVariablesToEngine(PropagationEngine& engine) {
    for (const auto& [identifier, modelVariable] : _model->variables()) {
      auto variable = _nodeMap->at(identifier);
      const auto& [lb, ub] = variable->bounds();
      variable->setVarId(engine.makeIntVar(lb, lb, ub));
    }
  }

  [[nodiscard]] inline VarId engineVariable(
      const fznparser::IntVar& variable) const {
    EXPECT_TRUE(_nodeMap->contains(variable.identifier()));
    return _nodeMap->at(variable.identifier())->varId();
  }

  [[nodiscard]] inline VarId engineVariable(
      const fznparser::BoolVar& variable) const {
    EXPECT_TRUE(_nodeMap->contains(variable.identifier()));
    return _nodeMap->at(variable.identifier())->varId();
  }
};

inline void expectMarkedAsInput(
    invariantgraph::InvariantNodeId definingNode,
    const std::vector<invariantgraph::VarNodeId>& inputs) {
  for (const auto& variableNode : inputs) {
    EXPECT_NE(std::find(variableNode->inputFor().begin(),
                        variableNode->inputFor().end(), definingNode),
              variableNode->inputFor().end());
  }
}

enum class ConstraintType : unsigned char {
  NORMAL = 0,
  REIFIED = 1,
  CONSTANT_FALSE = 2,
  CONSTANT_TRUE = 3
};