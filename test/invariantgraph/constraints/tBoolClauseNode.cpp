#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/boolClauseNode.hpp"
#include "propagation/propagationEngine.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

static bool isViolating(const std::vector<Int>& asValues,
                        const std::vector<Int>& bsValues) {
  for (const Int aVal : asValues) {
    if (aVal != 0) {
      return true;
    }
  }
  for (const Int bVal : bsValues) {
    if (bVal == 0) {
      return true;
    }
  }
  return false;
}

template <ConstraintType Type>
class AbstractBoolClauseNodeTest : public NodeTestBase<BoolClauseNode> {
 public:
  VarNodeId a;
  VarNodeId b;
  VarNodeId c;
  VarNodeId d;
  VarNodeId r;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createBoolVar("a");
    b = createBoolVar("b");
    c = createBoolVar("c");
    d = createBoolVar("d");
    r = createBoolVar("r");

    fznparser::BoolVarArray as("");
    as.append(boolVar(a));
    as.append(boolVar(b));
    fznparser::BoolVarArray bs("");
    bs.append(boolVar(c));
    bs.append(boolVar(d));

    if constexpr (Type == ConstraintType::REIFIED) {
      _model->addConstraint(std::move(fznparser::Constraint(
          "bool_clause_reif", std::vector<fznparser::Arg>{
                                  as, bs, fznparser::BoolArg{boolVar(r)}})));

    } else {
      if constexpr (Type == ConstraintType::NORMAL) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "bool_clause", std::vector<fznparser::Arg>{as, bs})));
      } else if constexpr (Type == ConstraintType::CONSTANT_FALSE) {
        _model->addConstraint(std::move(fznparser::Constraint(
            "bool_clause_reif",
            std::vector<fznparser::Arg>{as, bs, fznparser::BoolArg{false}})));
      } else {
        _model->addConstraint(std::move(fznparser::Constraint(
            "bool_clause_reif",
            std::vector<fznparser::Arg>{as, bs, fznparser::BoolArg{true}})));
      }
    }

    makeInvNode(_model->constraints().front());
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().as().size(), 2);
    EXPECT_EQ(invNode().as().at(0), a);
    EXPECT_EQ(invNode().as().at(1), b);

    EXPECT_EQ(invNode().bs().size(), 2);
    EXPECT_EQ(invNode().bs().at(0), c);
    EXPECT_EQ(invNode().bs().at(1), d);

    EXPECT_EQ(invNode().staticInputVarNodeIds().size(),
              invNode().as().size() + invNode().bs().size());

    std::vector<VarNodeId> expectedVars(invNode().as());
    for (const auto& varNodeId : invNode().bs()) {
      expectedVars.emplace_back(varNodeId);
    }
    EXPECT_EQ(expectedVars, invNode().staticInputVarNodeIds());

    if constexpr (Type != ConstraintType::REIFIED) {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    } else {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
      EXPECT_EQ(invNode().reifiedViolationNodeId(), r);
    }
  }

  void application() {
    propagation::PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);
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

    // a, b, c and d
    EXPECT_EQ(engine.searchVariables().size(), 4);

    // a, b, c, d, sum
    EXPECT_EQ(engine.numVariables(), 5);

    // linear
    EXPECT_EQ(engine.numInvariants(), 1);
  }

  void propagation() {
    propagation::PropagationEngine engine;
    engine.open();
    addInputVarsToEngine(engine);
    invNode().registerOutputVariables(*_invariantGraph, engine);
    invNode().registerNode(*_invariantGraph, engine);

    EXPECT_EQ(invNode().as().size(), 2);
    std::vector<propagation::VarId> asInputs;
    for (const VarNodeId& inputVarNodeId : invNode().as()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      asInputs.emplace_back(varId(inputVarNodeId));
      engine.updateBounds(varId(inputVarNodeId), 0, 5, true);
    }
    EXPECT_EQ(invNode().bs().size(), 2);
    std::vector<propagation::VarId> bsInputs;
    for (const VarNodeId& inputVarNodeId : invNode().bs()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      bsInputs.emplace_back(varId(inputVarNodeId));
      engine.updateBounds(varId(inputVarNodeId), 0, 5, true);
    }

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId = invNode().violationVarId(*_invariantGraph);

    std::vector<Int> asValues(asInputs.size());
    std::vector<Int> bsValues(bsInputs.size());
    engine.close();

    for (asValues.at(0) = engine.lowerBound(asInputs.at(0));
         asValues.at(0) <= engine.upperBound(asInputs.at(0));
         ++asValues.at(0)) {
      for (asValues.at(1) = engine.lowerBound(asInputs.at(1));
           asValues.at(1) <= engine.upperBound(asInputs.at(1));
           ++asValues.at(1)) {
        for (bsValues.at(0) = engine.lowerBound(bsInputs.at(0));
             bsValues.at(0) <= engine.upperBound(bsInputs.at(0));
             ++bsValues.at(0)) {
          for (bsValues.at(1) = engine.lowerBound(bsInputs.at(1));
               bsValues.at(1) <= engine.upperBound(bsInputs.at(1));
               ++bsValues.at(1)) {
            engine.beginMove();
            for (size_t i = 0; i < asInputs.size(); ++i) {
              engine.setValue(asInputs.at(i), asValues.at(i));
            }
            for (size_t i = 0; i < bsInputs.size(); ++i) {
              engine.setValue(bsInputs.at(i), bsValues.at(i));
            }
            engine.endMove();

            engine.beginProbe();
            engine.query(violationId);
            engine.endProbe();
            if constexpr (Type != ConstraintType::CONSTANT_FALSE) {
              EXPECT_EQ(engine.currentValue(violationId) > 0,
                        isViolating(asValues, bsValues));
            } else {
              EXPECT_NE(engine.currentValue(violationId) > 0,
                        isViolating(asValues, bsValues));
            }
          }
        }
      }
    }
  }
};

class BoolClauseNodeTest
    : public AbstractBoolClauseNodeTest<ConstraintType::NORMAL> {};

TEST_F(BoolClauseNodeTest, Construction) { construction(); }

TEST_F(BoolClauseNodeTest, Application) { application(); }

TEST_F(BoolClauseNodeTest, Propagation) { propagation(); }

class BoolClauseReifNodeTest
    : public AbstractBoolClauseNodeTest<ConstraintType::REIFIED> {};

TEST_F(BoolClauseReifNodeTest, Construction) { construction(); }

TEST_F(BoolClauseReifNodeTest, Application) { application(); }

TEST_F(BoolClauseReifNodeTest, Propagation) { propagation(); }

class BoolClauseFalseNodeTest
    : public AbstractBoolClauseNodeTest<ConstraintType::CONSTANT_FALSE> {};

TEST_F(BoolClauseFalseNodeTest, Construction) { construction(); }

TEST_F(BoolClauseFalseNodeTest, Application) { application(); }

TEST_F(BoolClauseFalseNodeTest, Propagation) { propagation(); }

class BoolClauseTrueNodeTest
    : public AbstractBoolClauseNodeTest<ConstraintType::CONSTANT_TRUE> {};

TEST_F(BoolClauseTrueNodeTest, Construction) { construction(); }

TEST_F(BoolClauseTrueNodeTest, Application) { application(); }

TEST_F(BoolClauseTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing