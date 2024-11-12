#include <cstdlib>

#include "../viewTestHelper.hpp"
#include "atlantis/propagation/views/equalConst.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class EqualConstTest : public ViewTest {
 public:
  Int value{0};

  Int computeOutput(bool committedValue = false) {
    return std::abs((committedValue ? _solver->committedValue(inputVar)
                                    : _solver->currentValue(inputVar)) -
                    value);
  }

  void SetUp() override {
    inputVarLb = -5;
    inputVarUb = 5;
    ViewTest::SetUp();
  }

  void generate() {
    inputVarDist = std::uniform_int_distribution<Int>(inputVarLb, inputVarUb);
    _solver->open();
    makeInputVar();
    outputVar = _solver->makeIntView<EqualConst>(*_solver, inputVar, value);
    _solver->close();
  }
};

TEST_F(EqualConstTest, bounds) {
  std::vector<std::pair<Int, Int>> bounds{
      {-1000, -1000}, {-1000, 0}, {0, 0}, {0, 1000}, {1000, 1000}};
  std::vector<std::pair<Int, Int>> expected{
      {1000, 1000}, {0, 1000}, {0, 0}, {0, 1000}, {1000, 1000}};

  EXPECT_EQ(bounds.size(), expected.size());
  generate();

  for (size_t i = 0; i < bounds.size(); ++i) {
    const auto& [inputLb, inputUb] = bounds.at(i);
    EXPECT_LE(inputLb, inputUb);

    const auto& [expectedLb, expectedUb] = expected.at(i);
    EXPECT_LE(expectedLb, expectedUb);

    _solver->updateBounds(VarId(inputVar), inputLb, inputUb, false);

    EXPECT_EQ(_solver->lowerBound(outputVar), expectedLb);
    EXPECT_EQ(_solver->upperBound(outputVar), expectedUb);
  }
}

RC_GTEST_FIXTURE_PROP(EqualConstTest, rapidcheck, ()) {
  const Int v1 = *rc::gen::arbitrary<Int>();
  const Int v2 = *rc::gen::arbitrary<Int>();

  inputVarLb = std::min(v1, v2);
  inputVarUb = std::max(v1, v2);

  generate();

  const size_t numCommits = 3;
  const size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      _solver->setValue(inputVar, inputVarDist(gen));
      _solver->endMove();

      EXPECT_EQ(_solver->currentValue(outputVar), computeOutput());

      if (p == numProbes) {
        _solver->beginCommit();
      } else {
        _solver->beginProbe();
      }
      _solver->query(outputVar);
      if (p == numProbes) {
        _solver->endCommit();
      } else {
        _solver->endProbe();
      }
      RC_ASSERT(_solver->currentValue(outputVar) == computeOutput());
      RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));
    }
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));
  }
}

}  // namespace atlantis::testing
