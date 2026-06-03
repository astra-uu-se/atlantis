#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityClosedNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class GlobalCardinalityClosedNodeTestFixture
    : public NodeTestBase<GlobalCardinalityClosedNode> {
 protected:
  std::vector<Var> inputVars;
  const std::vector<Int> cover{2, 6};
  std::vector<Var> outputVars;

  Var reifiedVar{"reified", std::vector<Int>{}, true};

  bool isViolating(const bool isRegistered = false) {
    if (isRegistered) {
      std::vector<Int> counts(cover.size(), 0);
      for (const auto& var : inputVars) {
        const Int val = varNode(var).isFixed()
                            ? varNode(var).lowerBound()
                            : _solver->currentValue(varId(var));
        bool valInCover = false;
        for (size_t i = 0; i < cover.size(); ++i) {
          if (val == cover.at(i)) {
            counts.at(i)++;
            valInCover = true;
          }
        }
        if (!valInCover) {
          return true;
        }
      }
      for (size_t i = 0; i < counts.size(); ++i) {
        if (counts.at(i) != _solver->currentValue(varId(outputVars.at(i)))) {
          return true;
        }
      }
      return false;
    }
    std::vector<Int> counts(cover.size(), 0);
    for (const auto& var : inputVars) {
      const Int val = varNode(var).lowerBound();
      bool valInCover = false;
      for (size_t i = 0; i < cover.size(); ++i) {
        if (val == cover.at(i)) {
          counts.at(i)++;
          valInCover = true;
          break;
        }
      }
      if (!valInCover) {
        return true;
      }
    }
    for (size_t i = 0; i < counts.size(); ++i) {
      if (counts.at(i) != _solver->currentValue(varId(outputVars.at(i)))) {
        return true;
      }
    }
    return false;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVars =
        std::vector<Var>{Var{"input_1", {}, true}, Var{"input_2", {}, true}};
    if (shouldBeSubsumed()) {
      inputVars.at(0).domain = std::pair<Int, Int>{0, 2};
      inputVars.at(1).domain = std::vector<Int>{1, 3, 5};
    } else if (shouldBeReplaced()) {
      inputVars.at(0).domain = std::pair<Int, Int>{1, 3};
      inputVars.at(1).domain = std::pair<Int, Int>{1, 3};
    } else {
      inputVars.at(0).domain = std::pair<Int, Int>{1, 5};
      inputVars.at(1).domain = std::pair<Int, Int>{1, 5};
    }
    for (const auto& var : inputVars) {
      retrieveIntVarNode(var);
    }

    outputVars.clear();
    for (size_t i = 0; i < cover.size(); ++i) {
      outputVars.emplace_back("output_" + std::to_string(i + 1), 0,
                              static_cast<Int>(inputVars.size()), true);
      retrieveIntVarNode(outputVars.back());
    }

    if (isReified()) {
      reifiedVar.domain = std::vector<Int>{0, 1};
      retrieveBoolVarNode(reifiedVar);
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          std::vector<Int>{cover}, varNodeIds(outputVars),
                          varNodeId(reifiedVar));
    } else {
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          std::vector<Int>{cover}, varNodeIds(outputVars),
                          shouldHold());
    }
  }
};

TEST_P(GlobalCardinalityClosedNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    const bool expected = isViolating();
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVar).isFixed());
      const bool actual = varNode(reifiedVar).inDomain(false);
      EXPECT_EQ(expected, actual);
    }
    if (shouldHold()) {
      EXPECT_FALSE(expected);
    }
    if (shouldFail()) {
      EXPECT_TRUE(expected);
    }
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : inputVars) {
    if (!varNode(var).isFixed()) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  const propagation::VarViewId violVarId =
      isReified() ? varId(reifiedVar) : _solverMapping->totalViolationId();

  EXPECT_NE(violVarId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(violVarId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const bool actual = _solver->currentValue(violVarId) > 0;
    const bool expected = isViolating(true);

    if (!shouldFail()) {
      EXPECT_EQ(actual, expected);
    } else {
      EXPECT_NE(actual, expected);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    GlobalCardinalityLowUpNodeTest, GlobalCardinalityClosedNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
