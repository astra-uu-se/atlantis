#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpClosedNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class GlobalCardinalityLowUpClosedNodeTestFixture
    : public NodeTestBase<GlobalCardinalityLowUpClosedNode> {
 protected:
  std::vector<Var> inputVars;
  const std::vector<Int> cover{2, 6};
  const std::vector<Int> low{0, 1};
  const std::vector<Int> up{1, 2};
  Var reifiedVar{"reified", std::vector<Int>{}, false};

  [[nodiscard]] bool isViolating(const bool isRegistered = false) const {
    if (isRegistered) {
      std::vector<Int> counts(cover.size(), 0);
      for (const auto& var : inputVars) {
        bool valInCover = false;
        const Int val = varNodeConst(var).isFixed()
                            ? varNodeConst(var).lowerBound()
                            : _solver->currentValue(varId(var));
        for (size_t i = 0; i < cover.size(); ++i) {
          if (val == cover.at(i)) {
            valInCover = true;
            counts.at(i)++;
            break;
          }
        }
        if (!valInCover) {
          return true;
        }
      }
      for (size_t i = 0; i < counts.size(); ++i) {
        if (counts.at(i) < low.at(i) || up.at(i) < counts.at(i)) {
          return true;
        }
      }
      return false;
    }
    std::vector<Int> counts(cover.size(), 0);
    for (const auto& var : inputVars) {
      const Int val = varNodeConst(var).lowerBound();
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
      if (counts.at(i) < low.at(i) || up.at(i) < counts.at(i)) {
        return true;
      }
    }
    return false;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVars = std::vector<Var>{Var{"input_1", std::vector<Int>{}, true},
                                 Var{"input_2", std::vector<Int>{}, true}};
    if (shouldBeSubsumed()) {
      if (shouldHold()) {
        inputVars.at(0).domain = std::pair<Int, Int>{5, 10};
        inputVars.at(1).domain = std::pair<Int, Int>{2, 7};
      } else {
        inputVars.at(0).domain = std::pair<Int, Int>{2, 10};
        inputVars.at(1).domain = std::pair<Int, Int>{3, 5};
      }
    } else {
      inputVars.at(0).domain = std::pair<Int, Int>{2, 10};
      inputVars.at(1).domain = std::pair<Int, Int>{2, 7};
    }

    for (const auto& var : inputVars) {
      retrieveIntVarNode(var);
    }

    if (isReified()) {
      reifiedVar.domain = std::vector<Int>{0, 1};
      retrieveBoolVarNode(reifiedVar);
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          std::vector<Int>{cover}, std::vector<Int>{low},
                          std::vector<Int>{up}, varNodeId(reifiedVar));
    } else {
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          std::vector<Int>{cover}, std::vector<Int>{low},
                          std::vector<Int>{up}, shouldHold());
    }
  }
};

TEST_P(GlobalCardinalityLowUpClosedNodeTestFixture, propagation) {
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

  const propagation::VarViewId violVarId =
      isReified() ? varId(reifiedVar) : _solverMapping->totalViolationId();

  EXPECT_NE(violVarId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVars);

  while (increaseNextVal(inputVars, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVars, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(violVarId);
    _solver->endProbe();

    expectVarVals(inputVars, inputVals);

    const bool actual = violVarId == propagation::NULL_ID
                            ? false
                            : _solver->currentValue(violVarId) > 0;
    const bool expected = isViolating(true);

    if (!shouldFail()) {
      EXPECT_EQ(actual, expected);
    } else {
      EXPECT_NE(actual, expected);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    GlobalCardinalityLowUpClosedNodeTest,
    GlobalCardinalityLowUpClosedNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_FALSE}));

}  // namespace atlantis::testing
