#include "invariantgraph/variableNode.hpp"

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
              SearchDomain({std::get<Int>(*var.value)})};
        } else if constexpr (std::is_same_v<Type, bool>) {
          if (std::get<bool>(*var.value)) {
            return std::optional<SearchDomain>{SearchDomain({0})};
          } else {
            return std::optional<SearchDomain>{SearchDomain({1})};
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
                      return SearchDomain(std::numeric_limits<Int>::min(),
                                          std::numeric_limits<Int>::max());
                    },
                    [](const fznparser::IntRange& range) {
                      return SearchDomain(range.lowerBound, range.upperBound);
                    },
                    [](const fznparser::LiteralSet<Int>& set) {
                      return SearchDomain(set.values);
                    }},
                var.domain);
          },
          [](const fznparser::BoolVariable&) {
            return SearchDomain(std::vector<Int>{0, 1});
          },
      },
      variable);
}

invariantgraph::VariableNode::VariableNode(VariableNode::FZNVariable variable)
    : _variable(std::move(variable)), _domain{convertDomain(*_variable)} {}

invariantgraph::VariableNode::VariableNode(SearchDomain domain)
    : _variable(std::nullopt), _domain(std::move(domain)) {}

VarId invariantgraph::VariableNode::postDomainConstraint(
    Engine& engine, std::vector<DomainEntry>&& domain) {
  if (domain.size() == 0 || _domainViolationId != NULL_ID) {
    return _domainViolationId;
  }
  _domainViolationId = engine.makeIntVar(0, 0, 0);

  const size_t interval =
      domain.back().upperBound - domain.front().lowerBound + 1;

  // domain.size() - 1 = number of "holes" in the domain:
  const float denseness = 1.0 - ((float)(domain.size() - 1) / (float)interval);
  if (SPARSE_MIN_DENSENESS <= denseness) {
    engine.makeConstraint<InSparseDomain>(_domainViolationId, this->varId(),
                                          std::move(domain));
  } else {
    engine.makeConstraint<InDomain>(_domainViolationId, this->varId(),
                                    std::move(domain));
  }
  return _domainViolationId;
}

std::vector<DomainEntry> invariantgraph::VariableNode::constrainedDomain(
    const Engine& engine) {
  assert(this->varId() != NULL_ID);
  const VarId varId = this->varId();
  return _domain.relativeComplementIfIntersects(engine.lowerBound(varId),
                                                engine.upperBound(varId));
}
