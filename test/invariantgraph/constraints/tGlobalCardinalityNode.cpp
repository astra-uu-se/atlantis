#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/violationInvariantNodes/globalCardinalityNode.hpp"

static std::vector<Int> computeOutputs(const std::vector<Int>& values,
                                       const std::vector<Int>& cover) {
  std::vector<Int> outputs(cover.size(), 0);
  for (size_t i = 0; i < values.size(); ++i) {
    for (size_t j = 0; j < cover.size(); ++j) {
      if (values.at(i) == cover.at(j)) {
        outputs.at(j)++;
      }
    }
  }
  return outputs;
}

static bool isSatisfied(const std::vector<Int>& values,
                        const std::vector<Int>& cover,
                        const std::vector<Int>& counts) {
  auto outputs = computeOutputs(values, cover);
  if (outputs.size() != counts.size()) {
    return false;
  }
  for (size_t i = 0; i < counts.size(); ++i) {
    if (outputs.at(i) != counts.at(i)) {
      return false;
    }
  }
  return true;
}

template <ConstraintType Type>
class AbstractGlobalCardinalityNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> x1;
  std::unique_ptr<fznparser::IntVar> x2;
  const std::vector<Int> cover{2, 6};
  std::unique_ptr<fznparser::IntVar> o1;
  std::unique_ptr<fznparser::IntVar> o2;
  std::unique_ptr<fznparser::BoolVar> r;

  std::unique_ptr<fznparser::Constraint> constraint;
  std::unique_ptr<fznparser::Model> model;
  std::unique_ptr<invariantgraph::GlobalCardinalityNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = intVar(5, 10, "x1");
    x2 = intVar(2, 7, "x2");
    o1 = intVar(1, 2, "o1");
    o2 = intVar(1, 2, "o2");
    r = boolVar("r");

    fznparser::IntVarArray inputsArg("");
    inputsArg.append(*x1);
    inputsArg.append(*x2);

    fznparser::IntVarArray coversArg("");
    for (const Int val : cover) {
      coversArg.append(val);
    }

    fznparser::IntVarArray outputsArg("");
    outputsArg.append(*o1);
    outputsArg.append(*o2);

    if constexpr (Type == ConstraintType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "fzn_global_cardinality_reif",
          std::vector<fznparser::Arg>{inputsArg, coversArg, outputsArg,
                                      fznparser::BoolArg{*r}})));

    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_global_cardinality_closed",
            std::vector<fznparser::Arg>{inputsArg, coversArg, outputsArg})));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_global_cardinality_reif",
            std::vector<fznparser::Arg>{inputsArg, coversArg, outputsArg,
                                        fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_global_cardinality_reif",
            std::vector<fznparser::Arg>{inputsArg, coversArg, outputsArg,
                                        fznparser::BoolArg{true}})));
      }
    }

    node = makeNode<invariantgraph::GlobalCardinalityNode>(
        _model->constraints().front());
  }

  void construction() {
    size_t numInputs = 2;
    size_t numOutputs = 2;
    if (Type == ConstraintType::REIFIED ||
        Type == ConstraintType::CONSTANT_FALSE) {
      numInputs += 2;
      numOutputs -= 2;
    }
    EXPECT_EQ(node->staticInputVarNodeIds().size(), numInputs);
    std::vector<invariantgraph::VarNodeId> expectedInputs{_nodeMap->at("x1"),
                                                          _nodeMap->at("x2")};
    EXPECT_EQ(node->staticInputVarNodeIds(), expectedInputs);
    EXPECT_THAT(expectedInputs,
                testing::ContainerEq(node->staticInputVarNodeIds()));
    expectMarkedAsInput(node.get(), node->staticInputVarNodeIds());

    if (Type == ConstraintType::REIFIED) {
      EXPECT_EQ(node->outputVarNodeIds().size(), 1);
      EXPECT_EQ(node->outputVarNodeIds().front(), _nodeMap->at("r"));
    } else {
      EXPECT_EQ(node->outputVarNodeIds().size(), numOutputs);
      std::vector<invariantgraph::VarNodeId> expectedOutputs{
          _nodeMap->at("o1"), _nodeMap->at("o2")};
      EXPECT_EQ(node->outputVarNodeIds(), expectedOutputs);
      EXPECT_THAT(expectedOutputs,
                  testing::ContainerEq(node->outputVarNodeIds()));
    }

    if constexpr (Type == ConstraintType::REIFIED) {
      EXPECT_TRUE(node->isReified());
      EXPECT_NE(node->reifiedViolation(), nullptr);
      EXPECT_EQ(node->reifiedViolation()->variable(),
                invariantgraph::VarNode::FZNVariable(*r));
    } else {
      EXPECT_FALSE(node->isReified());
      EXPECT_EQ(node->reifiedViolation(), nullptr);
    }
  }

  void application() {
    PropagationEngine engine;
    engine.open();
    addVariablesToEngine(engine);

    for (auto* const definedVariable : node->outputVarNodeIds()) {
      EXPECT_EQ(definedVariable->varId(), NULL_ID);
    }
    EXPECT_EQ(node->violationVarId(), NULL_ID);
    node->registerOutputVariables(engine);
    for (auto* const definedVariable : node->outputVarNodeIds()) {
      EXPECT_NE(definedVariable->varId(), NULL_ID);
    }
    if constexpr (Type == ConstraintType::NORMAL ||
                  Type == ConstraintType::CONSTANT_TRUE) {
      EXPECT_EQ(node->violationVarId(), NULL_ID);
    } else {
      EXPECT_NE(node->violationVarId(), NULL_ID);
    }

    node->registerNode(*_invariantGraph, engine);
    engine.close();

    if constexpr (Type == ConstraintType::NORMAL ||
                  Type == ConstraintType::CONSTANT_TRUE) {
      // x1, x2
      EXPECT_EQ(engine.searchVariables().size(), 2);
      // x1, x2, o1, o2
      EXPECT_EQ(engine.numVariables(), 4);
      // gcc
      EXPECT_EQ(engine.numInvariants(), 1);
    } else {
      // x1, x2, o1, o2
      EXPECT_EQ(engine.searchVariables().size(), 4);
      // x1, x2, o1, o2
      // intermediate o1, intermediate o2
      // 2 intermediate violations
      // 1 total violation
      EXPECT_EQ(engine.numVariables(), 9);
      // gcc + 2 (non)-equal + 1 total violation
      EXPECT_EQ(engine.numInvariants(), 4);

      EXPECT_EQ(engine.lowerBound(node->violationVarId()), 0);
      EXPECT_GT(engine.upperBound(node->violationVarId()), 0);
    }
  }

  void propagation() {
    PropagationEngine engine;
    engine.open();
    addVariablesToEngine(engine);
    node->registerOutputVariables(engine);
    node->registerNode(*_invariantGraph, engine);

    std::vector<VarId> inputs;
    for (auto* const input : node->inputs()) {
      EXPECT_NE(input->varId(), NULL_ID);
      inputs.emplace_back(input->varId());
    }
    EXPECT_EQ(inputs.size(), 2);

    std::vector<VarId> counts;
    for (auto* const count : node->counts()) {
      EXPECT_NE(count->varId(), NULL_ID);
      counts.emplace_back(count->varId());
    }
    EXPECT_EQ(counts.size(), 2);

    if constexpr (Type == ConstraintType::NORMAL ||
                  Type == ConstraintType::CONSTANT_TRUE) {
      EXPECT_EQ(node->violationVarId(), NULL_ID);
    } else {
      EXPECT_NE(node->violationVarId(), NULL_ID);
    }
    const VarId violationId = node->violationVarId();

    std::vector<Int> inputVals(inputs.size());
    std::vector<Int> countVals(counts.size());

    std::vector<std::pair<Int, Int>> countBounds;

    if constexpr (Type == ConstraintType::NORMAL ||
                  Type == ConstraintType::CONSTANT_TRUE) {
      for (const VarId c : counts) {
        countBounds.emplace_back(std::pair<Int, Int>{0, 0});
      }
    } else {
      for (const VarId c : counts) {
        countBounds.emplace_back(
            std::pair<Int, Int>{engine.lowerBound(c), engine.lowerBound(c)});
      }
    }

    engine.close();

    for (inputVals.at(0) = engine.lowerBound(inputs.at(0));
         inputVals.at(0) <= engine.upperBound(inputs.at(0));
         ++inputVals.at(0)) {
      for (inputVals.at(1) = engine.lowerBound(inputs.at(1));
           inputVals.at(1) <= engine.upperBound(inputs.at(1));
           ++inputVals.at(1)) {
        for (countVals.at(0) = countBounds.at(0).first;
             countVals.at(0) <= countBounds.at(0).second; ++countVals.at(0)) {
          for (countVals.at(1) = countBounds.at(1).first;
               countVals.at(1) <= countBounds.at(1).second; ++countVals.at(1)) {
            engine.beginMove();
            for (size_t i = 0; i < inputs.size(); ++i) {
              engine.setValue(inputs.at(i), inputVals.at(i));
            }
            if constexpr (Type == ConstraintType::CONSTANT_FALSE ||
                          Type == ConstraintType::REIFIED) {
              for (size_t i = 0; i < counts.size(); ++i) {
                engine.setValue(counts.at(i), countVals.at(i));
              }
            }
            engine.endMove();

            engine.beginProbe();
            engine.query(violationId);
            engine.endProbe();
            if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
              if constexpr (Type == ConstraintType::REIFIED) {
                EXPECT_EQ(engine.currentValue(violationId) == 0,
                          isSatisfied(inputVals, cover, countVals));
              } else {
                const std::vector<Int> actual =
                    computeOutputs(inputVals, cover);
                EXPECT_EQ(countVals.size(), counts.size());
                for (size_t i = 0; i < countVals.size(); ++i) {
                  EXPECT_EQ(engine.currentValue(counts.at(i)), actual.at(i));
                }
              }
            } else {
              EXPECT_NE(engine.currentValue(violationId) == 0,
                        isSatisfied(inputVals, cover, countVals));
            }
          }
        }
      }
    }
  }
};

class GlobalCardinalityNodeTest
    : public AbstractGlobalCardinalityNodeTest<ConstraintType::NORMAL> {};

TEST_F(GlobalCardinalityNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityNodeTest, Propagation) { propagation(); }

class GlobalCardinalityReifNodeTest
    : public AbstractGlobalCardinalityNodeTest<ConstraintType::REIFIED> {};

TEST_F(GlobalCardinalityReifNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityReifNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityReifNodeTest, Propagation) { propagation(); }

class GlobalCardinalityFalseNodeTest
    : public AbstractGlobalCardinalityNodeTest<ConstraintType::CONSTANT_FALSE> {
};

TEST_F(GlobalCardinalityFalseNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityFalseNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityFalseNodeTest, Propagation) { propagation(); }

class GlobalCardinalityTrueNodeTest
    : public AbstractGlobalCardinalityNodeTest<ConstraintType::CONSTANT_TRUE> {
};

TEST_F(GlobalCardinalityTrueNodeTest, Construction) { construction(); }

TEST_F(GlobalCardinalityTrueNodeTest, Application) { application(); }

TEST_F(GlobalCardinalityTrueNodeTest, Propagation) { propagation(); }