#include "../viewTestHelper.hpp"
#include "atlantis/propagation/views/modView.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class ModViewTest : public ViewTest {
 public:
  Int denominator{1};

  void SetUp() override {
    inputVarLb = -5;
    inputVarUb = 5;
    ViewTest::SetUp();
  }

  void generate() {
    _solver->open();
    makeInputVar();
    outputVar = _solver->makeIntView<ModView>(*_solver, inputVar, denominator);
    _solver->close();
  }

  Int computeOutput(bool committedValue = false) {
    return (committedValue ? _solver->committedValue(inputVar)
                           : _solver->currentValue(inputVar)) %
           denominator;
  }
};

TEST_F(ModViewTest, bounds) {
  std::vector<std::pair<Int, Int>> bounds{
      {-1000, -1000}, {-1000, 0}, {0, 0}, {0, 1000}, {1000, 1000}};
  std::vector<Int> values{-1000, -1, 1, 1000};

  for (const auto v : values) {
    denominator = v;
    generate();

    for (size_t i = 0; i < bounds.size(); ++i) {
      const auto& [inputLb, inputUb] = bounds.at(i);
      EXPECT_LE(inputLb, inputUb);

      _solver->updateBounds(VarId(inputVar), inputLb, inputUb, false);

      EXPECT_EQ(_solver->lowerBound(outputVar), 0);
      EXPECT_EQ(_solver->upperBound(outputVar), std::abs(v) - 1);
    }
  }
}

RC_GTEST_FIXTURE_PROP(ModViewTest, rapidcheck, ()) {
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
