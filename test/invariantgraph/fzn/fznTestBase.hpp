#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <deque>
#include <fznparser/constraint.hpp>
#include <fznparser/model.hpp>
#include <fznparser/types.hpp>
#include <fznparser/variables.hpp>
#include <random>
#include <string>
#include <vector>

#include "invariantgraph/fznInvariantGraph.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace fznparser;
using namespace atlantis::invariantgraph;

class FznTestBase : public ::testing::Test {
 public:
  std::unique_ptr<Model> _model;
  std::unique_ptr<FznInvariantGraph> _invariantGraph;
  std::unique_ptr<propagation::Solver> _solver;
  std::string constraintIdentifier{""};

  void SetUp() override {
    _model = std::make_unique<Model>();
    _invariantGraph = std::make_unique<FznInvariantGraph>();
    _solver = std::make_unique<propagation::Solver>();
  }

  Var fznVar(const std::string& identifier) { return _model->var(identifier); }

  VarNodeId varNodeId(const std::string& identifier) {
    return _invariantGraph->varNodeId(identifier);
  }

  propagation::VarId varId(const std::string& identifier) {
    return _invariantGraph->varId(identifier);
  }

  void setValue(const std::string& identifier, Int val) {
    _solver->setValue(varId(identifier), val);
  }

  Int currentValue(const std::string& identifier) {
    return _solver->currentValue(varId(identifier));
  }

  propagation::VarId totalViolationVarId() {
    return _invariantGraph->totalViolationVarId();
  }

  Int violation() { return _solver->currentValue(totalViolationVarId()); }

  std::vector<Int> makeInputVals(
      const std::vector<std::string>& inputIdentifiers) {
    std::vector<Int> inputVals;
    inputVals.reserve(inputIdentifiers.size());
    for (const std::string& identifier : inputIdentifiers) {
      inputVals.emplace_back(_solver->lowerBound(varId(identifier)));
    }
    return inputVals;
  }

  bool increaseNextVal(const std::vector<std::string>& inputIdentifiers,
                       std::vector<Int>& inputVals) {
    EXPECT_EQ(inputIdentifiers.size(), inputVals.size());
    for (Int i = inputVals.size() - 1; i >= 0; --i) {
      if (inputVals.at(i) <
          _solver->upperBound(varId(inputIdentifiers.at(i)))) {
        ++inputVals.at(i);
        return true;
      }
      inputVals.at(i) = _solver->lowerBound(varId(inputIdentifiers.at(i)));
    }
    return false;
  }

  void setVarVals(const std::vector<std::string>& inputIdentifiers,
                  const std::vector<Int>& vals) {
    EXPECT_EQ(inputIdentifiers.size(), vals.size());
    for (size_t i = 0; i < inputIdentifiers.size(); ++i) {
      _solver->setValue(varId(inputIdentifiers.at(i)), vals.at(i));
    }
  }

  std::vector<propagation::VarId> outputVarIds(
      const std::vector<std::string>& inputIdentifiers) {
    for (const std::string& inputIdentifier : inputIdentifiers) {
      EXPECT_NE(varId(inputIdentifier), propagation::NULL_ID);
      const std::vector<InvariantNodeId>& listeningInvNodeIds =
          _invariantGraph->varNode(inputIdentifier).inputTo();

      EXPECT_GE(listeningInvNodeIds.size(), 1);
      if (listeningInvNodeIds.size() > 1) {
        continue;
      }
      const std::vector<VarNodeId> outputVarNodeIds =
          _invariantGraph->invariantNode(listeningInvNodeIds.front())
              .outputVarNodeIds();
      std::vector<propagation::VarId> outputVarIds;
      outputVarIds.reserve(outputVarNodeIds.size());
      for (const VarNodeId& outputVarNodeId : outputVarNodeIds) {
        outputVarIds.emplace_back(
            _invariantGraph->varNode(outputVarNodeId).varId());
      }
      return outputVarIds;
    }
    return std::vector<propagation::VarId>{};
  }
};

}  // namespace atlantis::testing