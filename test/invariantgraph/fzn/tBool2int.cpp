#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/bool2int.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class bool2intTest : public FznTestBase {
 public:
  std::string boolVar{"boolVar"};
  std::string intVar{"intVar"};
  bool boolDefinesInt{true};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected = boolVal(boolVar, committedValue);
    const bool actual = intVal(intVar, committedValue) == 0;
    return actual == expected;
  }

  void generate() override {
    const auto& boolArg = addBoolArg(boolVar);
    const auto& intArg = addIntArg(intVar);
    annotations.clear();
    if (!boolArg.isParameter() && !intArg.isParameter()) {
      boolDefinesInt = *rc::gen::arbitrary<bool>();
      annotations.emplace_back("defines_var",
                               std::vector<std::vector<AnnotationExpression>>{
                                   std::vector<AnnotationExpression>{Annotation{
                                       boolDefinesInt ? intVar : boolVar}}});
    } else if (!boolArg.isParameter()) {
      RC_ASSERT(intArg.isParameter());
      boolDefinesInt = *rc::gen::arbitrary<bool>();
      if (!boolDefinesInt) {
        annotations.emplace_back(
            "defines_var",
            std::vector<std::vector<AnnotationExpression>>{
                std::vector<AnnotationExpression>{Annotation{boolVar}}});
      }
    } else if (!intArg.isParameter()) {
      RC_ASSERT(boolArg.isParameter());
      boolDefinesInt = *rc::gen::arbitrary<bool>();
      if (boolDefinesInt) {
        annotations.emplace_back(
            "defines_var",
            std::vector<std::vector<AnnotationExpression>>{
                std::vector<AnnotationExpression>{Annotation{intVar}}});
      }
    }

    constraintIdentifier = "bool2int";
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    return 0 <= upperBound(intVar) && lowerBound(intVar) <= 1;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (upperBound(intVar) < 0 || 1 < lowerBound(intVar)) {
      return true;
    }
    if (isFixed(boolVar) && isFixed(intVar)) {
      return boolVal(boolVar) != (intVal(intVar) == 1);
    }
    return false;
  }

  [[nodiscard]] bool canMove() const override {
    return varId(boolVar) != propagation::NULL_ID;
  }

  void move(bool committedValue) override {
    if (varId(boolVar) != propagation::NULL_ID && randBool()) {
      changeValue(boolVar, committedValue);
    }
  }

  void query() override {
    _solver->query(totalViolationVarId() != propagation::NULL_ID
                       ? totalViolationVarId()
                       : varId(intVar));
  }
};

RC_GTEST_FIXTURE_PROP(bool2intTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing