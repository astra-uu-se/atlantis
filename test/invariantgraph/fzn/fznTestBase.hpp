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

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace fznparser;
using namespace atlantis::invariantgraph;

class FznTestBase : public ::testing::Test {
 public:
  std::unique_ptr<Model> _model;
  std::unique_ptr<FznInvariantGraph> _invariantGraph;
  std::unique_ptr<propagation::Solver> _solver;
  std::string constraintIdentifier;

  void SetUp() override {
    _model = std::make_unique<Model>();
    _invariantGraph = std::make_unique<FznInvariantGraph>();
    _solver = std::make_unique<propagation::Solver>();
  }

  [[nodiscard]] VarNodeId varNodeId(const std::string& identifier) const {
    return _invariantGraph->varNodeId(identifier);
  }

  [[nodiscard]] propagation::VarId varId(const std::string& identifier) const {
    return _invariantGraph->varId(identifier);
  }

  void setValue(const std::string& identifier, Int val) const {
    _solver->setValue(varId(identifier), val);
  }

  [[nodiscard]] Int currentValue(const std::string& identifier) const {
    return _solver->currentValue(varId(identifier));
  }

  [[nodiscard]] propagation::VarId totalViolationVarId() const {
    return _invariantGraph->totalViolationVarId();
  }

  [[nodiscard]] Int violation() const {
    return _solver->currentValue(totalViolationVarId());
  }

  [[nodiscard]] std::vector<Int> makeInputVals(
      const std::vector<std::string>& inputIdentifiers) const {
    std::vector<Int> inputVals;
    inputVals.reserve(inputIdentifiers.size());
    for (const std::string& identifier : inputIdentifiers) {
      inputVals.emplace_back(_solver->lowerBound(varId(identifier)));
    }
    return inputVals;
  }

  bool increaseNextVal(const std::vector<std::string>& inputIdentifiers,
                       std::vector<Int>& inputVals) const {
    EXPECT_EQ(inputIdentifiers.size(), inputVals.size());
    for (Int i = static_cast<Int>(inputVals.size() - 1); i >= 0; --i) {
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
                  const std::vector<Int>& vals) const {
    EXPECT_EQ(inputIdentifiers.size(), vals.size());
    for (size_t i = 0; i < inputIdentifiers.size(); ++i) {
      _solver->setValue(varId(inputIdentifiers.at(i)), vals.at(i));
    }
  }
};

}  // namespace atlantis::testing
