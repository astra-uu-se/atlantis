#pragma once

#include <optional>
#include <unordered_set>
#include <vector>

#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

// Undefine the DOMAIN macro that is defined in math.h on MacOS
#undef DOMAIN

namespace atlantis::invariantgraph {

class VarNode {
 public:
  enum struct DomainType : unsigned char {
    NONE = 0,
    FIXED = 1,
    LOWER_BOUND = 2,
    UPPER_BOUND = 3,
    RANGE = 4,
    DOMAIN = 5
  };

 private:
  VarNodeId _varNodeId;
  bool _isIntVar;
  DomainType _domainType{DomainType::DOMAIN};
  SearchDomain _domain;
  propagation::VarViewId _varId{propagation::NULL_ID};
  propagation::VarViewId _domainViolationId{propagation::NULL_ID};

  std::vector<InvariantNodeId> _staticInputTo;
  std::vector<InvariantNodeId> _dynamicInputTo;
  std::unordered_set<InvariantNodeId, InvariantNodeIdHash> _outputOf;
  bool _isViolationVar{false};

 public:
  /**
   * Construct a variable node which is not associated with a model variable.
   *
   * @param domain The domain of this variable.
   */
  explicit VarNode(VarNodeId, bool isIntVar,
                   VarNode::DomainType = DomainType::RANGE);
  /**
   * Construct a variable node which is not associated with a model variable.
   *
   * @param domain The domain of this variable.
   */
  explicit VarNode(VarNodeId, bool isIntVar, SearchDomain&& domain,
                   VarNode::DomainType = DomainType::DOMAIN);

  VarNodeId varNodeId() const noexcept;

  /**
   * @return The model propagation::VarViewId this node is associated
   * with, or propagation::NULL_ID if no propagation::VarViewId is
   * associated with this node.
   */
  [[nodiscard]] propagation::VarViewId varId() const;

  /**
   * @return The model propagation::VarViewId this node is associated
   * with, or propagation::NULL_ID if no propagation::VarViewId is
   * associated with this node.
   */
  void setVarId(propagation::VarViewId varId);

  [[nodiscard]] const SearchDomain& constDomain() const noexcept;

  [[nodiscard]] SearchDomain& domain() noexcept;
  [[nodiscard]] const SearchDomain& domainConst() const noexcept;

  [[nodiscard]] bool isFixed() const noexcept;

  [[nodiscard]] bool isIntVar() const noexcept;
  [[nodiscard]] bool isViolationVar() const noexcept;

  [[nodiscard]] Int lowerBound() const;
  [[nodiscard]] Int upperBound() const;
  [[nodiscard]] Int val() const;

  [[nodiscard]] bool inDomain(Int) const;
  [[nodiscard]] bool inDomain(bool) const;

  void setIsViolationVar(bool isViolVar);

  /**
   * @brief removes the given value from the domain of the variable.
   */
  void removeValue(Int);

  /**
   * @brief fixes the variable to the given value.
   */
  void fixToValue(Int);

  /**
   * @brief removes all values that are strictly less than the given value from
   * the domain of the variable.
   */
  void removeValuesBelow(Int);

  /**
   * @brief removes all values that are strictly greater than the given value
   * from the domain of the variable.
   */
  void removeValuesAbove(Int);

  /**
   * @brief removes all values that are not in the given vector from the domain
   * from the domain of the variable.
   */
  void removeValues(const std::vector<Int>&);

  /**
   * @brief removes all values that are not in the given vector from the domain
   * from the domain of the variable.
   */
  void removeAllValuesExcept(const std::vector<Int>&);

  /**
   * @brief fixes the variable to the given value.
   */
  void removeValue(bool);

  /**
   * @brief fixes the variable to the given value.
   */
  void fixToValue(bool);

  DomainType domainType() const noexcept;

  void tightenDomainType(DomainType);
  void setDomainType(DomainType);

  /**
   * @return if the bound range of the corresponding IntVar in solver is a
   * sub-set of SearchDomain _domain, then returns an empty vector,
   * otherwise the relative complement of varLb..varUb in SearchDomain is
   * returned
   */
  [[nodiscard]] std::vector<DomainEntry> constrainedDomain(Int lb, Int ub);

  propagation::VarViewId postDomainConstraint(propagation::SolverBase&);

  [[nodiscard]] std::pair<Int, Int> bounds() const;

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] const std::vector<InvariantNodeId>& staticInputTo()
      const noexcept;

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] const std::vector<InvariantNodeId>& dynamicInputTo()
      const noexcept;

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] const std::unordered_set<InvariantNodeId, InvariantNodeIdHash>&
  definingNodes() const noexcept;

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] InvariantNodeId outputOf() const;

  /**
   * Indicate this variable node serves as an input to the given variable
   * defining node.
   *
   * @param node The variable defining node for which this is an input.
   */
  void markAsInputFor(InvariantNodeId listeningInvariant, bool isStaticInput);

  /**
   * Indicate this variable node is no longer defined by the variable defining
   * node.
   *
   * @param node The variable defining node for which this is no longer defined.
   */
  void unmarkOutputTo(InvariantNodeId definingInvariant);

  /**
   * Indicate this variable node no longer serves as an input to the given
   * variable defining node.
   *
   * @param node The variable defining node for which this is no longer an
   * input.
   */
  void unmarkAsInputFor(InvariantNodeId listeningInvariant, bool isStaticInput);

  /**
   * Indicate this variable node is defined by the given variable
   * defining node.
   *
   * @param node The variable defining node this is defined by.
   */
  void markOutputTo(InvariantNodeId definingInvariant);

  /**
   * @return The constant value if this node represents a node, or std::nullopt
   * otherwise.
   */
  [[nodiscard]] std::optional<Int> constantValue() const noexcept;

  std::ostream& dotLangIdentifier(std::ostream&,
                                  const std::string& identifier) const;
};

}  // namespace atlantis::invariantgraph
