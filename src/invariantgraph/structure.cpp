#include "invariantgraph/structure.hpp"

#include <limits>

using namespace invariantgraph;

static SearchDomain convertDomain(const VariableNode::FZNVariable& variable) {
  auto singletonDomain = std::visit<std::optional<SearchDomain>>(
      overloaded{[]<typename Type>(const fznparser::SearchVariable<Type>& var) {
        // TODO: The value could be an identifier to a parameter.
        if (!var.value ||
            std::holds_alternative<fznparser::Identifier>(*var.value)) {
          return std::optional<SearchDomain>{};
        }

        if constexpr (std::is_same_v<Type, Int>) {
          return std::optional<SearchDomain>{
              SearchDomain(SetDomain({std::get<Int>(*var.value)}))};
        } else if constexpr (std::is_same_v<Type, bool>) {
          if (std::get<bool>(*var.value)) {
            return std::optional<SearchDomain>{SearchDomain(SetDomain({0}))};
          } else {
            return std::optional<SearchDomain>{SearchDomain(SetDomain({1}))};
          }
        } else {
          static_assert(!sizeof(Type),
                        "Only Int and bool variables are allowed");
        }
      }},
      variable);

  if (singletonDomain) {
    return *singletonDomain;
  }

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

VariableNode::VariableNode(SearchDomain domain)
    : _variable(std::nullopt), _domain(std::move(domain)) {}

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

VarId VariableNode::postDomainConstraint(Engine& engine,
                                         const VariableMap& variableMap,
                                         std::vector<DomainEntry>&& domain) {
  if (domain.size() == 0 || _domainViolationId != NULL_ID) {
    return _domainViolationId;
  }
  _domainViolationId = engine.makeIntVar(0, 0, 0);

  const size_t interval =
      domain.back().upperBound - domain.front().lowerBound + 1;

  // domain.size() - 1 = number of "holes" in the domain:
  const float denseness = 1.0 - ((float)(domain.size() - 1) / (float)interval);
  if (SPARSE_MIN_DENSENESS <= denseness) {
    engine.makeConstraint<InSparseDomain>(
        _domainViolationId, variableMap.at(this), std::move(domain));
  } else {
    engine.makeConstraint<InDomain>(_domainViolationId, variableMap.at(this),
                                    std::move(domain));
  }
  return _domainViolationId;
}

std::vector<DomainEntry> VariableNode::constrainedDomain(
    const Engine& engine, const VariableMap& variableMap) {
  assert(variableMap.contains(this));
  const VarId varId = variableMap.at(this);
  return std::visit<std::vector<DomainEntry>>(
      [&](const auto& dom) {
        return dom.relativeComplementIfIntersects(engine.lowerBound(varId),
                                                  engine.upperBound(varId));
      },
      _domain);
}
