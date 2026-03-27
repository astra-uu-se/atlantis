#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class GlobalCardinalityLowUpNodeTestFixture
    : public NodeTestBase<GlobalCardinalityLowUpNode> {
 public:
  std::vector<std::string> inputVars;
  const std::vector<Int> cover{2, 6};
  const std::vector<Int> low{0, 1};
  const std::vector<Int> up{1, 2};
  std::string reifiedVar{"reified"};

  bool isViolating(bool isRegistered = false) {
    if (isRegistered) {
      std::vector<Int> counts(cover.size(), 0);
      for (const auto& var : inputVars) {
        const Int val = varNode(var).isFixed()
                            ? varNode(var).lowerBound()
                            : _solver->currentValue(varId(var));
        for (size_t i = 0; i < cover.size(); ++i) {
          if (val == cover.at(i)) {
            counts.at(i)++;
            break;
          }
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
      const Int val = varNode(var).lowerBound();
      for (size_t i = 0; i < cover.size(); ++i) {
        if (val == cover.at(i)) {
          counts.at(i)++;
          break;
        }
      }
    }
    for (size_t i = 0; i < counts.size(); ++i) {
      if (counts.at(i) < low.at(i) || up.at(i) < counts.at(i)) {
        return true;
      }
    }
    return false;
  }

  void SetUp() {
    NodeTestBase::SetUp();
    inputVars = {"x_0", "x_1"};
    retrieveIntVarNode(5, 10, inputVars.at(0));

    retrieveIntVarNode(2, 7, inputVars.at(1));

    if (isReified()) {
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

TEST_P(GlobalCardinalityLowUpNodeTestFixture, propagation) {
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

INSTANTIATE_TEST_CASE_P(
    GlobalCardinalityLowUpNodeTest, GlobalCardinalityLowUpNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
