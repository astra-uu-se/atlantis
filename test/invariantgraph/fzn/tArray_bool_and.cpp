#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_bool_and.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class array_bool_andTest : public FznTestBase {
 public:
  std::vector<std::string> inputIdentifiers{};
  std::string outputIdentifier = "output";

  bool isSatisfied(const std::vector<Int>& inputVals) {
    return std::all_of(inputVals.begin(), inputVals.end(),
                       [](Int val) { return val == 0; });
  }

  void generate(const std::vector<Int>& inputs, Int output) {
    inputIdentifiers.reserve(inputs.size());
    std::vector<fznparser::Arg> args;
    args.reserve(2);

    for (size_t i = 0; i < inputs.size(); ++i) {
      inputIdentifiers.emplace_back("b_" + std::to_string(i));
      if (inputs[i] < 0) {
        _model->addVar(
            std::make_shared<fznparser::BoolVar>(inputIdentifiers.back()));
      } else {
        _model->addVar(std::make_shared<fznparser::BoolVar>(
            inputs[i] > 0, inputIdentifiers.back()));
      }
    }
    if (output < 0) {
      _model->addVar(std::make_shared<fznparser::BoolVar>(outputIdentifier));
    } else {
      _model->addVar(
          std::make_shared<fznparser::BoolVar>(output > 0, outputIdentifier));
    }

    auto inputsArg = std::make_shared<fznparser::BoolVarArray>("inputs");
    for (size_t i = 0; i < inputs.size(); ++i) {
      inputsArg->append(std::get<std::shared_ptr<fznparser::BoolVar>>(
          _model->var(inputIdentifiers.at(i))));
    }
    args.emplace_back(inputsArg);
    args.emplace_back(std::get<std::shared_ptr<fznparser::BoolVar>>(
        _model->var(outputIdentifier)));

    _model->addConstraint(Constraint(constraintIdentifier, std::move(args)));
  }

  void SetUp() override {
    FznTestBase::SetUp();
    constraintIdentifier = "array_bool_and";
  }

  void generateSimple() {
    std::vector<Int> inputs(4, -1);
    generate(inputs, -1);
  }
};

TEST_F(array_bool_andTest, construction) {
  generateSimple();
  EXPECT_EQ(_model->constraints().size(), 1);
  EXPECT_TRUE(array_bool_and(*_invariantGraph, _model->constraints().front()));
  for (const auto& identifier : inputIdentifiers) {
    EXPECT_TRUE(_invariantGraph->containsVarNode(identifier));
    EXPECT_NE(_invariantGraph->varNodeId(identifier), NULL_NODE_ID);
  }
  EXPECT_TRUE(_invariantGraph->containsVarNode(outputIdentifier));
  EXPECT_NE(_invariantGraph->varNodeId(outputIdentifier), NULL_NODE_ID);
}

TEST_F(array_bool_andTest, propagation) {
  generateSimple();
  array_bool_and(*_invariantGraph, _model->constraints().front());
  _invariantGraph->construct();
  _invariantGraph->close();

  std::vector<propagation::VarViewId> inputVarIds = getVarIds(inputIdentifiers);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(varId(outputIdentifier));
    _solver->endProbe();

    const bool actual = currentValue(outputIdentifier) == 0;
    const bool expected = isSatisfied(inputVals);

    EXPECT_EQ(actual, expected);
  }
}

}  // namespace atlantis::testing
