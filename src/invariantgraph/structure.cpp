#include "invariantgraph/structure.hpp"

#include <limits>

#include "utils/variant.hpp"

using namespace invariantgraph;

static SearchDomain convertDomain(const VariableNode::FZNVariable& variable) {
  return std::visit<SearchDomain>(
      overloaded{
          [](const fznparser::IntVariable& var) {
            return std::visit<SearchDomain>(
                overloaded{
                    [](const fznparser::BasicDomain<Int>&) {
                      return IntervalDomain(std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());
                    },
                    [](const fznparser::IntRange& range) {
                      return IntervalDomain(range.lowerBound, range.upperBound);
                    },
                    [](const fznparser::LiteralSet<Int>& set) {
                      return SetDomain(set.values);
                    }},
                var.domain);
          },
          [](const fznparser::BoolVariable&) {
            return SetDomain({0, 1});
          },
      },
      variable);
}

VariableNode::VariableNode(VariableNode::FZNVariable variable)
    : _variable(std::move(variable)), _domain{convertDomain(*_variable)} {}

void ImplicitConstraintNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  std::vector<search::SearchVariable> varIds;
  varIds.reserve(definedVariables().size());

  for (const auto& node : definedVariables()) {
    const auto& [lb, ub] = node->bounds();
    auto varId = engine.makeIntVar(lb, lb, ub);

    variableMap.emplace(node, varId);
    varIds.emplace_back(varId, node->domain());
  }

  _neighbourhood = createNeighbourhood(engine, varIds);
  assert(_neighbourhood);
}
