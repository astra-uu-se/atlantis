#include "nodeTestBase.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

Var::Var(std::string i, std::vector<Int>&& d, const bool iv)
    : identifier(std::move(i)), domain(std::move(d)), isIntVar(iv) {}
Var Var::IntVar(const std::string& identifier, std::vector<Int>&& d) {
  return Var(identifier, std::move(d), true);
}
Var Var::BoolVar(const std::string& identifier) {
  return Var(identifier, std::vector<Int>{0, 1}, false);
}
Var Var::BoolVar(const std::string& identifier, const bool val) {
  return Var(identifier, std::vector<Int>{val == true ? 1 : 0}, false);
}
size_t Var::size() const {
  if (std::holds_alternative<std::vector<Int>>(domain)) {
    return std::get<std::vector<Int>>(domain).size();
  }
  const auto [lb, ub] = std::get<std::pair<Int, Int>>(domain);
  EXPECT_LE(lb, ub);
  return static_cast<size_t>(ub - lb + 1);
}
Int Var::val() const {
  EXPECT_EQ(size(), 1);
  if (std::holds_alternative<std::vector<Int>>(domain)) {
    return std::get<std::vector<Int>>(domain).front();
  }
  return std::get<std::pair<Int, Int>>(domain).first;
}
void Var::fixToValue(const bool value) { fixToValue(Int{value ? 1 : 0}); }
ParamData::ParamData(InvariantNodeAction a, ViolationInvariantType vt, int d)
    : action(a), violType(vt), data(d) {}
ParamData::ParamData(InvariantNodeAction a, ViolationInvariantType vt)
    : action(a), violType(vt), data(0) {}
ParamData::ParamData(InvariantNodeAction a, int d)
    : action(a), violType(ViolationInvariantType::CONSTANT_TRUE), data(d) {}
ParamData::ParamData(InvariantNodeAction a)
    : action(a), violType(ViolationInvariantType::CONSTANT_TRUE), data(0) {}
ParamData::ParamData(ViolationInvariantType vt, int d)
    : action(InvariantNodeAction::NONE), violType(vt), data(d) {}
ParamData::ParamData(ViolationInvariantType vt)
    : action(InvariantNodeAction::NONE), violType(vt), data(0) {}
ParamData::ParamData(int d)
    : action(InvariantNodeAction::NONE),
      violType(ViolationInvariantType::CONSTANT_TRUE),
      data(d) {}
ParamData::ParamData()
    : action(InvariantNodeAction::NONE),
      violType(ViolationInvariantType::CONSTANT_TRUE),
      data(0) {}
}  // namespace atlantis::testing