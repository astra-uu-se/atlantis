#pragma once

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/types.hpp"
#include "solverMapping.hpp"

namespace atlantis {
class SortedUniqueVector;
}
namespace atlantis {
class SearchDomain;  // forward declaration
}

namespace atlantis::propagation {
class SolverBase;  // forward declaration
}

namespace atlantis::invariantgraph {

class VarNode {
  VarNodeId _varNodeId;
  bool _isIntVar;
  DomainType _domainType{DomainType::DOM_DOMAIN};
  std::shared_ptr<SearchDomain> _domain;

  std::vector<InvariantNodeId> _staticInputTo;
  std::vector<InvariantNodeId> _dynamicInputTo;
  std::unordered_set<InvariantNodeId, InvariantNodeIdHash> _outputOf;
  bool _isViolationVar{false};
  std::optional<std::string> _identifier;

 public:
  explicit VarNode(VarNodeId, bool isIntVar,
                   DomainType = DomainType::DOM_RANGE);

  explicit VarNode(VarNodeId, bool isIntVar,
                   const std::shared_ptr<SearchDomain>& domain,
                   DomainType = DomainType::DOM_DOMAIN);

  explicit VarNode(const std::string& identifier, VarNodeId, bool isIntVar,
                   DomainType = DomainType::DOM_RANGE);

  explicit VarNode(const std::string& identifier, VarNodeId, bool isIntVar,
                   const std::shared_ptr<SearchDomain>& domain,
                   DomainType = DomainType::DOM_DOMAIN);

  VarNodeId varNodeId() const noexcept;

  [[nodiscard]] std::shared_ptr<const SearchDomain> constDomain()
      const noexcept;

  [[nodiscard]] std::shared_ptr<SearchDomain> domain() noexcept;

  [[nodiscard]] bool isFixed() const noexcept;

  [[nodiscard]] bool isIntVar() const noexcept;
  [[nodiscard]] bool isViolationVar() const noexcept;

  [[nodiscard]] Int lowerBound() const;
  [[nodiscard]] Int upperBound() const;
  [[nodiscard]] Int val() const;

  [[nodiscard]] bool inDomain(Int) const;
  [[nodiscard]] bool inDomain(bool) const;

  void removeValue(bool);

  void fixToValue(bool);

  void setIsViolationVar(bool isViolVar);

  void removeValue(Int, bool tightenDomainState = true);

  void fixToValue(Int, bool tightenDomainState = true);

  void removeValuesBelow(Int, bool tightenDomainState = true);

  void removeValuesAbove(Int, bool tightenDomainState = true);

  void removeValues(const SortedUniqueVector&, bool tightenDomainState = true);

  void removeAllValuesExcept(const SortedUniqueVector&,
                             bool tightenDomainState = true);

  DomainType domainType() const noexcept;

  void tightenDomainType(DomainType);
  void setDomainType(DomainType);

  [[nodiscard]] std::vector<DomainEntry> constrainedDomain(Int lb,
                                                           Int ub) const;

  propagation::VarViewId postDomainConstraint(propagation::SolverBase&,
                                              SolverMapping&) const;

  [[nodiscard]] std::pair<Int, Int> bounds() const;

  [[nodiscard]] const std::vector<InvariantNodeId>& staticInputTo()
      const noexcept;

  [[nodiscard]] const std::vector<InvariantNodeId>& dynamicInputTo()
      const noexcept;

  [[nodiscard]] const std::unordered_set<InvariantNodeId, InvariantNodeIdHash>&
  definingNodes() const noexcept;

  [[nodiscard]] InvariantNodeId outputOf() const;

  void markAsInputFor(InvariantNodeId listeningInvNodeId, bool isStaticInput);

  void unmarkOutputTo(InvariantNodeId definingInvNodeId);

  void unmarkAsInputFor(InvariantNodeId listeningInvariant, bool isStaticInput);

  void markOutputTo(InvariantNodeId definingInvariant);

  [[nodiscard]] std::optional<Int> constantValue() const noexcept;

  std::ostream& dotLangIdentifier(std::ostream&,
                                  const std::string& identifier) const;
};

}  // namespace atlantis::invariantgraph
