#pragma once

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/types.hpp"

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
class InvariantNode;
class SolverMapping;

class VarNode : public std::enable_shared_from_this<VarNode> {
  DomainType _domainType{DomainType::DOM_DOMAIN};
  bool _isViolationVar{false};
  bool _isOutputVar{false};
  bool _isIntVar;
  ConstraintVarId _constraintSolverId{NULL_NODE_ID};
  VarNodeId _mappingId{NULL_NODE_ID};

  std::shared_ptr<SearchDomain> _domain{nullptr};
  std::vector<std::shared_ptr<InvariantNode>> _staticInputTo;
  std::vector<std::shared_ptr<InvariantNode>> _dynamicInputTo;
  std::unordered_set<std::shared_ptr<InvariantNode>> _outputOf;
  std::optional<std::string> _identifier;

 public:
  explicit VarNode(
      bool isIntVar,
      ConstraintVarId constraintVarId = ConstraintVarId{NULL_NODE_ID},
      DomainType = DomainType::DOM_RANGE);

  explicit VarNode(
      bool isIntVar, const std::shared_ptr<SearchDomain>& domain,
      ConstraintVarId constraintVarId = ConstraintVarId{NULL_NODE_ID},
      DomainType = DomainType::DOM_DOMAIN);

  explicit VarNode(
      const std::string& identifier, bool isIntVar,
      ConstraintVarId constraintVarId = ConstraintVarId{NULL_NODE_ID},
      DomainType = DomainType::DOM_RANGE);

  explicit VarNode(const std::string& identifier, bool isIntVar,
                   const std::shared_ptr<SearchDomain>& domain,
                   ConstraintVarId constraintVarId = {NULL_NODE_ID, false},
                   DomainType = DomainType::DOM_DOMAIN);

  std::shared_ptr<VarNode> ptr();

  std::shared_ptr<const VarNode> constPtr() const;

  void setMappingId(size_t) noexcept;
  size_t mappingId() const noexcept;

  ConstraintVarId constraintVarId() const noexcept;

  void setConstraintVarId(ConstraintVarId constraintVarId);

  void replaceDomain(const std::shared_ptr<SearchDomain>& newDomain);

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

  void setIsViolationVar(bool);

  [[nodiscard]] bool isOutputVar() const noexcept;

  void setIsOutputVar(bool);

  void removeValue(Int, bool tightenDomainState = true);

  void fixToValue(Int, bool tightenDomainState = true);

  void removeValuesBelow(Int, bool tightenDomainState = true);

  void removeValuesAbove(Int, bool tightenDomainState = true);

  void removeValues(const SortedUniqueVector&, bool tightenDomainState = true);

  void removeAllValuesExcept(const SortedUniqueVector&,
                             bool tightenDomainState = true);

  DomainType domainType() const noexcept;

  void tightenDomainType();

  void tightenDomainType(DomainType);
  void setDomainType(DomainType);

  [[nodiscard]] std::vector<DomainEntry> constrainedDomain(Int lb,
                                                           Int ub) const;

  propagation::VarViewId postDomainConstraint(propagation::SolverBase&,
                                              SolverMapping&) const;

  [[nodiscard]] std::pair<Int, Int> bounds() const;

  [[nodiscard]] const std::vector<std::shared_ptr<InvariantNode>>& staticInputTo()
      const noexcept;

  [[nodiscard]] const std::vector<std::shared_ptr<InvariantNode>>& dynamicInputTo()
      const noexcept;

  [[nodiscard]] const std::unordered_set<std::shared_ptr<InvariantNode>>&
  definingNodes() const noexcept;

  [[nodiscard]] std::shared_ptr<InvariantNode> outputOf() const;

  void markAsInputFor(const std::shared_ptr<InvariantNode>& listeningInvariant, bool isStaticInput);

  void markOutputTo(const std::shared_ptr<InvariantNode>& definingInvariant);

  void unmarkAsInputFor(const InvariantNode& listeningInvariant, bool isStaticInput);

  void unmarkOutputTo(const std::shared_ptr<InvariantNode>& definingInvNodeId);

  [[nodiscard]] std::optional<Int> constantValue() const noexcept;

  std::ostream& dotLangIdentifier(std::ostream&,
                                  const std::string& identifier) const;
};

}  // namespace atlantis::invariantgraph
