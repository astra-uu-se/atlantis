#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <iostream>
#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/int_lin_eq.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class int_lin_eqTest : public FznTestBase {
 public:
  std::vector<std::string> inputs{};
  std::vector<Int> coeffs{};
  std::string reified{"reified"};
  Int bound{0};
  Int definedIndex{-1};

  [[nodiscard]] Int getFixedLHS() const {
    Int total = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (isFixed(inputs.at(i))) {
        total += coeffs.at(i) * intVal(inputs.at(i));
      }
    }
    return total;
  }

  [[nodiscard]] bool sameCoeff(Int& coeff) const {
    bool initialized = false;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (isFixed(inputs.at(i)) || coeffs.at(i) == 0) {
        continue;
      }
      if (initialized && coeff != std::abs(coeffs.at(i))) {
        return false;
      }
      initialized = true;
      coeff = std::abs(coeffs.at(i));
    }
    return initialized;
  }

  [[nodiscard]] std::pair<Int, Int> getBounds() const {
    Int lb = 0;
    Int ub = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (coeffs.at(i) == 0) {
        continue;
      }
      if (isFixed(inputs.at(i))) {
        lb += intVal(inputs.at(i)) * coeffs.at(i);
        ub += intVal(inputs.at(i)) * coeffs.at(i);
      } else {
        const Int vLb = lowerBound(inputs.at(i)) * coeffs.at(i);
        const Int vUb = upperBound(inputs.at(i)) * coeffs.at(i);
        lb += std::min<Int>(vLb, vUb);
        ub += std::max<Int>(vLb, vUb);
      }
    }
    return {lb, ub};
  }

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    Int sum = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (coeffs.at(i) != 0) {
        sum += coeffs.at(i) * intVal(inputs.at(i), committedValue);
      }
    }
    const bool expected = sum == bound;
    const bool actual = boolVal(reified, committedValue);

    if (isFixed(reified)) {
      const bool satAssignment = violation(committedValue) == 0;
      return satAssignment ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    const auto [lb, ub] = getBounds();
    if (lb == ub) {
      if (!isFixed(reified)) {
        return true;
      }
      if (isFixedTo(reified, true)) {
        return lb == bound;
      }
      return lb != bound;
    }
    const bool alwaysSat = lb == ub && lb == bound;
    if (alwaysSat) {
      return isFixedTo(reified, bool{true});
    }
    const bool alwaysUnsat = ub < bound || bound < lb;
    if (alwaysUnsat) {
      return isFixedTo(reified, bool{false});
    }
    const Int total = bound - getFixedLHS();
    Int coeff;
    if (sameCoeff(coeff) && total % coeff != 0) {
      return isFixedTo(reified, bool{false});
    }
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    const auto [lb, ub] = getBounds();
    const bool alwaysSat = lb == ub && lb == bound;
    if (alwaysSat) {
      return isFixedTo(reified, bool{false});
    }
    const bool alwaysUnsat = ub < bound || bound < lb;
    if (alwaysUnsat) {
      return isFixedTo(reified, bool{true});
    }
    const Int total = bound - getFixedLHS();
    Int coeff;
    if (sameCoeff(coeff) && total % coeff != 0) {
      return isFixedTo(reified, bool{true});
    }
    return false;
  }

  void generate() override {
    const size_t size = *rc::gen::inRange<size_t>(0, 4);
    coeffs =
        *rc::gen::container<std::vector<Int>>(size, rc::gen::inRange(-2, 2));
    addArg(coeffs);
    inputs.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }
    const std::shared_ptr<IntVarArray> arr = addIntVarArray(inputs);
    std::vector<Int> varIndices;
    varIndices.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      if (std::holds_alternative<std::shared_ptr<const IntVar>>(arr->at(i))) {
        varIndices.emplace_back(i);
      }
    }

    Int lb = -1;
    Int ub = 2;
    for (const Int c : coeffs) {
      ub += std::max<Int>(0, c);
      lb += std::min<Int>(0, c);
    }

    bound = *rc::gen::inRange<Int>(lb, ub);
    addArg(bound);

    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier = isReified ? "int_lin_eq_reif" : "int_lin_eq";
    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    if (!isReified && !varIndices.empty()) {
      definedIndex = *rc::gen::elementOf(varIndices);
    } else {
      definedIndex = -1;
    }
    generateConstraint();
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(inputs, [&](const std::string& input) {
      return varId(input) != propagation::NULL_ID &&
             (definedIndex < 0 ||
              varId(input) != varId(inputs.at(definedIndex)));
    });
  }

  void move(bool committedValue) override {
    for (const auto& input : inputs) {
      if (varId(input) != propagation::NULL_ID &&
          (definedIndex < 0 ||
           varId(input) != varId(inputs.at(definedIndex))) &&
          randBool()) {
        changeValue(input, committedValue);
      }
    }
  }

  void query() override {
    if (varId(reified) != propagation::NULL_ID) {
      _solver->query(varId(reified));
    } else if (totalViolationVarId() != propagation::NULL_ID) {
      _solver->query(totalViolationVarId());
    }
    if (definedIndex >= 0 &&
        varId(inputs.at(definedIndex)) != propagation::NULL_ID) {
      _solver->query(varId(inputs.at(definedIndex)));
    }
  }
};

RC_GTEST_FIXTURE_PROP(int_lin_eqTest, RapidCheck, ()) { rapidCheck(); }

TEST(IntLinEqRegression, DefinedIntVarKeepsDeclaredDomainOnImport) {
  auto model = std::make_shared<fznparser::Model>();

  auto out = std::make_shared<fznparser::IntVar>(1, 4, "out");
  out->addAnnotation("is_defined_var");
  model->addVar(out);

  auto x = std::make_shared<fznparser::IntVar>(0, 10, "x");
  model->addVar(x);

  fznparser::Constraint constraint{
      "int_eq", std::vector<fznparser::Arg>{fznparser::IntArg(out),
                                            fznparser::IntArg(x)}};
  constraint.addAnnotation("defines_var", fznparser::AnnotationExpression(
                                              fznparser::Annotation("out")));
  model->addConstraint(std::move(constraint));

  auto graph = std::make_shared<FznInvariantGraph>(true);
  graph->open();
  ASSERT_NO_THROW(graph->build(*model));

  ASSERT_TRUE(graph->containsVarNode("out"));
  const auto& outNode = graph->varNodeConst("out");
  EXPECT_EQ(outNode.lowerBound(), 1);
  EXPECT_EQ(outNode.upperBound(), 4);
}

TEST(IntLinEqRegression, DefinedVarDomainDoesNotConflictWithDefinition) {
  auto model = std::make_shared<fznparser::Model>();

  auto out = std::make_shared<fznparser::IntVar>(1, 4, "out");
  out->addAnnotation("is_defined_var");
  model->addVar(out);

  auto a = std::make_shared<fznparser::IntVar>(1, "a");
  auto b = std::make_shared<fznparser::IntVar>(1, "b");
  auto c = std::make_shared<fznparser::IntVar>(1, "c");
  auto d = std::make_shared<fznparser::IntVar>(1, "d");
  model->addVar(a);
  model->addVar(b);
  model->addVar(c);
  model->addVar(d);

  auto coeffs = std::make_shared<fznparser::IntVarArray>("coeffs");
  coeffs->append(Int{1});
  coeffs->append(Int{-1});
  coeffs->append(Int{-1});
  coeffs->append(Int{-1});
  coeffs->append(Int{-1});

  auto vars = std::make_shared<fznparser::IntVarArray>("vars");
  vars->append(out);
  vars->append(a);
  vars->append(b);
  vars->append(c);
  vars->append(d);

  fznparser::Constraint constraint{
      "int_lin_eq",
      std::vector<fznparser::Arg>{coeffs, vars, fznparser::IntArg(Int{-2})}};
  constraint.addAnnotation("defines_var", fznparser::AnnotationExpression(
                                              fznparser::Annotation("out")));
  model->addConstraint(std::move(constraint));

  auto graph = std::make_shared<FznInvariantGraph>(true);
  graph->open();
  ASSERT_NO_THROW(graph->build(*model));
  graph->close();

  auto solver = std::make_shared<propagation::Solver>();
  EXPECT_NO_THROW(static_cast<void>(graph->construct(*solver)));
}

TEST(IntLinEqRegression, NonLinearDefinedIntVarDoesNotGetFullIntRange) {
  auto model = std::make_shared<fznparser::Model>();

  auto out = std::make_shared<fznparser::IntVar>(2, 5, "out");
  out->addAnnotation("is_defined_var");
  model->addVar(out);

  auto x = std::make_shared<fznparser::IntVar>(0, 10, "x");
  model->addVar(x);

  fznparser::Constraint constraint{
      "int_eq", std::vector<fznparser::Arg>{fznparser::IntArg(out),
                                            fznparser::IntArg(x)}};
  constraint.addAnnotation("defines_var", fznparser::AnnotationExpression(
                                              fznparser::Annotation("out")));
  model->addConstraint(std::move(constraint));

  auto graph = std::make_shared<FznInvariantGraph>(true);
  graph->open();
  ASSERT_NO_THROW(graph->build(*model));

  ASSERT_TRUE(graph->containsVarNode("out"));
  const auto& outNode = graph->varNodeConst("out");
  EXPECT_EQ(outNode.lowerBound(), 2);
  EXPECT_EQ(outNode.upperBound(), 5);
}

}  // namespace atlantis::testing
