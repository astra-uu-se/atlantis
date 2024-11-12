#include "../viewTestHelper.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class NotEqualConstTest : public ViewTest {
 public:
  Int value{0};

  void SetUp() override {
    inputVarLb = -5;
    inputVarUb = 5;
    ViewTest::SetUp();
  }

  void generate() {
    _solver->open();
    makeInputVar();
    outputVar = _solver->makeIntView<NotEqualConst>(*_solver, inputVar, value);
    _solver->close();
  }

  Int computeOutput(bool committedValue = false) {
    return value == (committedValue ? _solver->committedValue(inputVar)
                                    : _solver->currentValue(inputVar))
               ? 1
               : 0;
  }
};

TEST_F(NotEqualConstTest, bounds) {
  std::vector<std::pair<Int, Int>> bounds{
      {-1000, -1000}, {-1000, 0}, {0, 0}, {0, 1000}, {1000, 1000}};
  std::vector<Int> values{-1001, -1000, -999, -1, 0, 1, 999, 1000, 1001};

  for (const auto v : values) {
    value = v;
    generate();

    for (size_t i = 0; i < bounds.size(); ++i) {
      const auto& [inputLb, inputUb] = bounds.at(i);
      EXPECT_LE(inputLb, inputUb);

      _solver->updateBounds(VarId(inputVar), inputLb, inputUb, false);

      Int expectedLb = -1;
      Int expectedUb = -1;

      if (inputLb == value && value == inputUb) {
        expectedLb = 1;
        expectedUb = 1;
      } else if (inputLb <= value && value <= inputUb) {
        expectedLb = 0;
        expectedUb = 1;
      } else {
        expectedLb = 0;
        expectedUb = 0;
      }

      EXPECT_EQ(_solver->lowerBound(outputVar), expectedLb);
      EXPECT_EQ(_solver->upperBound(outputVar), expectedUb);
    }
  }
}

RC_GTEST_FIXTURE_PROP(NotEqualConstTest, rapidcheck, ()) {
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
