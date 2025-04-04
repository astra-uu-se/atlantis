#pragma once

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/types.hpp"

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
  propagation::VarViewId _varId{propagation::NULL_ID};
  propagation::VarViewId _domainViolationId{propagation::NULL_ID};

  std::vector<InvariantNodeId> _staticInputTo;
  std::vector<InvariantNodeId> _dynamicInputTo;
  std::unordered_set<InvariantNodeId, InvariantNodeIdHash> _outputOf;
  bool _isViolationVar{false};

 public:
  explicit VarNode(VarNodeId, bool isIntVar,
                   DomainType = DomainType::DOM_RANGE);
  explicit VarNode(VarNodeId, bool isIntVar,
                   const std::shared_ptr<SearchDomain>& domain,
                   DomainType = DomainType::DOM_DOMAIN);

  VarNodeId varNodeId() const noexcept;

  [[nodiscard]] propagation::VarViewId varId() const;

  void setVarId(propagation::VarViewId varId);

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

  void setIsViolationVar(bool isViolVar);

  void removeValue(Int);

  void fixToValue(Int);

  void removeValuesBelow(Int);

  void removeValuesAbove(Int);

  void removeValues(const std::vector<Int>&);

  void removeAllValuesExcept(const std::vector<Int>&);

  void removeValue(bool);

  void fixToValue(bool);

  DomainType domainType() const noexcept;

  void tightenDomainType(DomainType);
  void setDomainType(DomainType);

  [[nodiscard]] std::vector<DomainEntry> constrainedDomain(Int lb,
                                                           Int ub) const;

  propagation::VarViewId postDomainConstraint(propagation::SolverBase&);

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
