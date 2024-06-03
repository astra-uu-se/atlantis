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
  _invariantGraph->apply(*_solver);

  std::vector<Int> inputVals = makeInputVals(inputIdentifiers);

  while (increaseNextVal(inputIdentifiers, inputVals)) {
    _solver->beginMove();
    setVarVals(inputIdentifiers, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(varId(outputIdentifier));
    _solver->endProbe();

    const bool actual = currentValue(outputIdentifier) == 0;
    const bool expected = isSatisfied(inputVals);

    EXPECT_EQ(actual, expected);
  }
}

RC_GTEST_FIXTURE_PROP(array_bool_andTest, rapidcheck, ()) {
  const size_t numInputs = *rc::gen::inRange(0, 3);
  std::vector<fznparser::Arg> args;
  args.reserve(2);
  std::shared_ptr<fznparser::BoolVarArray> inputs = genBoolVarArray(numInputs);
  fznparser::BoolArg output = genBoolArg(outputIdentifier);
  args.emplace_back(inputs);
  args.emplace_back(output);
  _model->addConstraint(Constraint(constraintIdentifier, std::move(args)));

  bool allUnfixedVars = numInputs != 0;
  bool alwaysTrue = true;
  bool alwaysFalse = false;

  for (size_t i = 0; i < numInputs; ++i) {
    if (std::holds_alternative<bool>(inputs->at(i))) {
      alwaysTrue = alwaysTrue && std::get<bool>(inputs->at(i));
      alwaysFalse = alwaysFalse || !std::get<bool>(inputs->at(i));
      allUnfixedVars = false;
      continue;
    }
    std::shared_ptr<const fznparser::BoolVar> var =
        std::get<std::shared_ptr<const fznparser::BoolVar>>(inputs->at(i));

    RC_ASSERT(_model->hasVar(var->identifier()));
    RC_ASSERT(std::holds_alternative<std::shared_ptr<fznparser::BoolVar>>(
        _model->var(var->identifier())));

    if (var->isFixed()) {
      alwaysTrue = alwaysTrue && var->lowerBound();
      alwaysFalse = alwaysFalse || !var->lowerBound();
      allUnfixedVars = false;
    } else {
      alwaysTrue = false;
    }
  }
  if (alwaysTrue) {
    RC_ASSERT(!alwaysFalse);
  } else if (alwaysFalse) {
    RC_ASSERT(!alwaysTrue);
  }

  const bool outputIsFixed =
      std::holds_alternative<bool>(output) ||
      std::get<std::shared_ptr<const fznparser::BoolVar>>(output)->isFixed();
  const bool outputVal =
      outputIsFixed
          ? (std::holds_alternative<bool>(output)
                 ? std::get<bool>(output)
                 : std::get<std::shared_ptr<const fznparser::BoolVar>>(output)
                       ->lowerBound())
          : true;

  if (allUnfixedVars && outputIsFixed &&
      (alwaysTrue != outputVal || alwaysFalse != !outputVal)) {
    RC_ASSERT_THROWS_AS(
        array_bool_and(*_invariantGraph, _model->constraints().back()),
        FznArgumentException);
    RC_SUCCEED("Never holds");
  }

  RC_ASSERT(array_bool_and(*_invariantGraph, _model->constraints().back()));
  _invariantGraph->apply(*_solver);

  if (outputIsFixed) {
    RC_ASSERT(totalViolationVarId() != propagation::NULL_ID);
  } else {
    auto& outputVarNode = _invariantGraph->varNode(outputIdentifier);
    RC_ASSERT(outputVarNode.isFixed());
    if (alwaysTrue) {
      RC_ASSERT((outputVarNode.lowerBound() == 0) == alwaysTrue);
      return;
    } else if (alwaysFalse) {
      RC_ASSERT((outputVarNode.lowerBound() > 0) == alwaysFalse);
      return;
    }
  }
}

}  // namespace atlantis::testing
