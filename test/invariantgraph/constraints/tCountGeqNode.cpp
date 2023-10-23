#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/countGeqNode.hpp"
#include "propagation/propagationEngine.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

// Constrains c to be greater than or equal to the number of occurrences of y in
// values.
static bool isSatisfied(const std::vector<Int>& values, const Int y,
                        const Int c) {
  Int occurrences = 0;
  for (const Int val : values) {
    occurrences += (val == y ? 1 : 0);
  }
  return c >= occurrences;
}

template <bool YIsParameter, ConstraintType Type>
class AbstractCountGeqNodeTest : public NodeTestBase<CountGeqNode> {
 public:
  VarNodeId x1;
  VarNodeId x2;
  VarNodeId x3;
  VarNodeId y;
  VarNodeId c;
  VarNodeId r;
  const Int yParamVal{5};
  const Int cParamVal{2};

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = createIntVar(2, 5, "x1");
    x2 = createIntVar(3, 5, "x2");
    x3 = createIntVar(4, 5, "x3");
    y = createIntVar(2, 5, "y");
    c = createIntVar(0, 2, "c");
    r = createBoolVar("r");

    fznparser::IntVarArray inputs("");
    inputs.append(intVar(x1));
    inputs.append(intVar(x2));
    inputs.append(intVar(x3));

    if constexpr (Type == ConstraintType::REIFIED) {
      if constexpr (YIsParameter) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_count_geq_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{boolVar(r)}})));

      } else {
        // y is var
        _model->addConstraint(std::move(fznparser::Constraint(
            "fzn_count_geq_reif",
            std::vector<fznparser::Arg>{inputs, fznparser::IntArg{intVar(y)},
                                        fznparser::IntArg{cParamVal},
                                        fznparser::BoolArg{boolVar(r)}})));
      }
    } else {
      // No variable reification:
      if constexpr (Type == ConstraintType::NORMAL) {
        if constexpr (YIsParameter) {
          _model->addConstraint(std::move(fznparser::Constraint(
              "fzn_count_geq",
              std::vector<fznparser::Arg>{inputs, fznparser::IntArg{yParamVal},
                                          fznparser::IntArg{intVar(c)}})));

        } else {
          _model->addConstraint(std::move(fznparser::Constraint(
              "fzn_count_geq",
              std::vector<fznparser::Arg>{inputs, fznparser::IntArg{intVar(y)},
                                          fznparser::IntArg{intVar(c)}})));
        }
      } else {
        // constant reification:
        if constexpr (YIsParameter) {
          if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_geq_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{yParamVal},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{false}})));
          } else {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_geq_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{yParamVal},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{true}})));
          }
        } else {
          if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_geq_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{intVar(y)},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{false}})));
          } else {
            _model->addConstraint(std::move(fznparser::Constraint(
                "fzn_count_geq_reif",
                std::vector<fznparser::Arg>{
                    inputs, fznparser::IntArg{intVar(y)},
                    fznparser::IntArg{cParamVal}, fznparser::BoolArg{true}})));
          }
        }
      }
    }

    makeInvNode(_model->constraints().front());
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    std::vector<VarNodeId> expectedInputs{x1, x2, x3};
    if constexpr (YIsParameter) {
      EXPECT_EQ(invNode().yVarNode(), NULL_NODE_ID);
    } else {
      EXPECT_NE(invNode().yVarNode(), NULL_NODE_ID);
      expectedInputs.emplace_back(y);
    }
    if constexpr (Type != ConstraintType::NORMAL) {
      EXPECT_EQ(invNode().cVarNode(), NULL_NODE_ID);
    } else {
      EXPECT_NE(invNode().cVarNode(), NULL_NODE_ID);
      expectedInputs.emplace_back(c);
    }
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), expectedInputs.size());
    EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);
    EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

    std::vector<VarNodeId> expectedOutputs;
    if constexpr (Type == ConstraintType::REIFIED) {
      expectedOutputs.emplace_back(r);
    }

    EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
    EXPECT_THAT(expectedOutputs, ContainerEq(invNode().outputVarNodeIds()));

    if constexpr (Type == ConstraintType::REIFIED) {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
      EXPECT_EQ(invNode().reifiedViolationNodeId(), r);
    } else {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    }
  }

  void application() {
    propagation::PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);

    for (const auto varNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(engineVarId(varNodeId), propagation::NULL_ID);
    }
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_EQ(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerOutputVariables(*_invariantGraph, engine);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerNode(*_invariantGraph, engine);
    engine.close();

    // x1, x2, x3, y, c
    size_t numSearchVars = 5;
    // x1, x2, x3, y, c, intermediate
    size_t numVariables = 7;

    if constexpr (YIsParameter) {
      --numSearchVars;
      --numVariables;
    }
    if constexpr (Type != ConstraintType::NORMAL) {
      EXPECT_EQ(invNode().cVarNode(), NULL_NODE_ID);
      --numSearchVars;
      // violation and c
      numVariables -= 2;
    } else {
      EXPECT_NE(invNode().cVarNode(), NULL_NODE_ID);
    }
    EXPECT_EQ(engine.searchVariables().size(), numSearchVars);

    EXPECT_EQ(engine.numVariables(), numVariables);

    // countEq
    size_t numInvariants = 2;
    if constexpr (Type != ConstraintType::NORMAL) {
      --numInvariants;
    }

    EXPECT_EQ(engine.numInvariants(), numInvariants);
  }

  void propagation() {
    propagation::PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);
    invNode().registerOutputVariables(*_invariantGraph, engine);
    invNode().registerNode(*_invariantGraph, engine);

    std::vector<propagation::VarId> inputs;

    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputs.emplace_back(varId(inputVarNodeId));
    }

    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);
    EXPECT_NE(violationId, propagation::NULL_ID);
    if constexpr (Type == ConstraintType::REIFIED) {
      EXPECT_EQ(violationId, varId(r));
    }

    std::vector<Int> values;
    Int yLb = yParamVal;
    Int yUb = yParamVal;
    Int cLb = cParamVal;
    Int cUb = cParamVal;

    values.resize(3);
    const propagation::VarId yVar = invNode().yVarNode() == NULL_NODE_ID
                                        ? propagation::NULL_ID
                                        : varId(invNode().yVarNode());

    if constexpr (!YIsParameter) {
      EXPECT_NE(yVar, propagation::NULL_ID);
      yLb = engine.lowerBound(yVar);
      yUb = engine.upperBound(yVar);
    }

    const propagation::VarId cVar = invNode().cVarNode() == NULL_NODE_ID
                                        ? propagation::NULL_ID
                                        : varId(invNode().cVarNode());
    if constexpr (Type == ConstraintType::NORMAL) {
      EXPECT_NE(cVar, propagation::NULL_ID);
      cLb = engine.lowerBound(cVar);
      cUb = engine.upperBound(cVar);
    }
    engine.close();
    for (size_t i = 0; i < values.size(); ++i) {
      values.at(i) = engine.committedValue(inputs.at(i));
    }
    Int cVal = invNode().cVarNode() == NULL_NODE_ID
                   ? cLb
                   : engine.committedValue(cVar);
    Int yVal = invNode().yVarNode() == NULL_NODE_ID
                   ? yLb
                   : engine.committedValue(yVar);
    if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
      EXPECT_EQ(engine.committedValue(violationId) == 0,
                isSatisfied(values, yVal, cVal));
    } else {
      EXPECT_NE(engine.committedValue(violationId) == 0,
                isSatisfied(values, yVal, cVal));
    }

    for (values.at(0) = engine.lowerBound(inputs.at(0));
         values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
      for (values.at(1) = engine.lowerBound(inputs.at(1));
           values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
        for (values.at(2) = engine.lowerBound(inputs.at(2));
             values.at(2) <= engine.upperBound(inputs.at(2)); ++values.at(2)) {
          for (Int yVal = yLb; yVal <= yUb; ++yVal) {
            for (Int cVal = cLb; cVal <= cUb; ++cVal) {
              engine.beginMove();
              for (size_t i = 0; i < values.size(); ++i) {
                engine.setValue(inputs.at(i), values.at(i));
              }
              if constexpr (!YIsParameter) {
                engine.setValue(yVar, yVal);
              }
              if constexpr (Type == ConstraintType::NORMAL) {
                engine.setValue(cVar, cVal);
              }
              engine.endMove();

              engine.beginProbe();
              engine.query(violationId);
              engine.endProbe();
              if constexpr (!YIsParameter) {
                EXPECT_EQ(engine.currentValue(yVar), yVal);
              }
              if constexpr (Type == ConstraintType::NORMAL) {
                EXPECT_EQ(engine.currentValue(cVar), cVal);
              }

              const bool actual = engine.currentValue(violationId) == 0;
              const bool satisfied = isSatisfied(values, yVal, cVal);
              if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
                EXPECT_EQ(actual, satisfied);
              } else {
                EXPECT_NE(actual, satisfied);
              }
            }
          }
        }
      }
    }
  }
};

class CountGeqNodeTest
    : public AbstractCountGeqNodeTest<false, ConstraintType::NORMAL> {};

TEST_F(CountGeqNodeTest, Construction) { construction(); }

TEST_F(CountGeqNodeTest, Application) { application(); }

TEST_F(CountGeqNodeTest, Propagation) { propagation(); }

class CountGeqReifNodeTest
    : public AbstractCountGeqNodeTest<false, ConstraintType::REIFIED> {};

TEST_F(CountGeqReifNodeTest, Construction) { construction(); }

TEST_F(CountGeqReifNodeTest, Application) { application(); }

TEST_F(CountGeqReifNodeTest, Propagation) { propagation(); }

class CountGeqFalseNodeTest
    : public AbstractCountGeqNodeTest<false, ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountGeqFalseNodeTest, Construction) { construction(); }

TEST_F(CountGeqFalseNodeTest, Application) { application(); }

TEST_F(CountGeqFalseNodeTest, Propagation) { propagation(); }

class CountGeqTrueNodeTest
    : public AbstractCountGeqNodeTest<false, ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountGeqTrueNodeTest, Construction) { construction(); }

TEST_F(CountGeqTrueNodeTest, Application) { application(); }

TEST_F(CountGeqTrueNodeTest, Propagation) { propagation(); }

class CountGeqYParNodeTest
    : public AbstractCountGeqNodeTest<true, ConstraintType::NORMAL> {};

TEST_F(CountGeqYParNodeTest, Construction) { construction(); }

TEST_F(CountGeqYParNodeTest, Application) { application(); }

TEST_F(CountGeqYParNodeTest, Propagation) { propagation(); }

class CountGeqYParReifNodeTest
    : public AbstractCountGeqNodeTest<true, ConstraintType::REIFIED> {};

TEST_F(CountGeqYParReifNodeTest, Construction) { construction(); }

TEST_F(CountGeqYParReifNodeTest, Application) { application(); }

TEST_F(CountGeqYParReifNodeTest, Propagation) { propagation(); }

class CountGeqYParFalseNodeTest
    : public AbstractCountGeqNodeTest<true, ConstraintType::CONSTANT_FALSE> {};

TEST_F(CountGeqYParFalseNodeTest, Construction) { construction(); }

TEST_F(CountGeqYParFalseNodeTest, Application) { application(); }

TEST_F(CountGeqYParFalseNodeTest, Propagation) { propagation(); }

class CountGeqYParTrueNodeTest
    : public AbstractCountGeqNodeTest<true, ConstraintType::CONSTANT_TRUE> {};

TEST_F(CountGeqYParTrueNodeTest, Construction) { construction(); }

TEST_F(CountGeqYParTrueNodeTest, Application) { application(); }

TEST_F(CountGeqYParTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing