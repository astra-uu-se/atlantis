#pragma once

#include <cassert>
#include <fznparser/variables.hpp>
#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>

#include "invariantgraph/types.hpp"
#include "propagation/solver.hpp"
#include "propagation/views/inDomain.hpp"
#include "propagation/views/inSparseDomain.hpp"
#include "search/neighbourhoods/neighbourhood.hpp"
#include "search/searchVariable.hpp"
#include "utils/domains.hpp"
#include "utils/variant.hpp"

namespace atlantis::invariantgraph {

/**
 * The types that can be in an array of search variables.
 */
using MappableValue = std::variant<Int, bool, std::string>;

class InvariantGraph;  // Forward declaration
class InvariantNode;   // Forward declaration

/**
 * A variable in the invariant graph. Every variable is defined by a
 * InvariantNode. The variable is possibly associated with a model
 * variable.
 */
class VarNode {
 public:
  using FZNVar = std::variant<fznparser::BoolVar, fznparser::IntVar>;
  using VarMap =
      std::unordered_map<VarNodeId, propagation::VarId, VarNodeIdHash>;
  float SPARSE_MIN_DENSENESS{0.6};

 private:
  VarNodeId _varNodeId;
  std::optional<FZNVar> _var;
  SearchDomain _domain;
  const bool _isIntVar;
  propagation::VarId _varId{propagation::NULL_ID};
  propagation::VarId _domainViolationId{propagation::NULL_ID};

  std::vector<InvariantNodeId> _inputTo;
  std::vector<InvariantNodeId> _staticInputTo;
  std::vector<InvariantNodeId> _dynamicInputTo;
  std::unordered_set<InvariantNodeId, InvariantNodeIdHash> _outputOf;

 public:
  /**
   * Construct a variable node which is associated with a model variable.
   *
   * @param var The model variable this node is associated with.
   */
  explicit VarNode(VarNodeId, const FZNVar& var);

  /**
   * Construct a variable node which is not associated with a model variable.
   *
   * @param domain The domain of this variable.
   */
  explicit VarNode(VarNodeId, SearchDomain&& domain, bool isIntVar);

  /**
   * Construct a variable node which is not associated with a model variable.
   *
   * @param domain The domain of this variable.
   */
  explicit VarNode(VarNodeId, const SearchDomain& domain, bool isIntVar);

  VarNodeId varNodeId() const noexcept;

  /**
   * @return The model variable this node is associated with, or std::nullopt
   * if no model variable is associated with this node.
   */
  [[nodiscard]] std::optional<FZNVar> var() const;

  [[nodiscard]] std::string identifier() const;

  /**
   * @return The model propagation::VarId this node is associated
   * with, or propagation::NULL_ID if no propagation::VarId is
   * associated with this node.
   */
  [[nodiscard]] propagation::VarId varId() const;

  /**
   * @return The model propagation::VarId this node is associated
   * with, or propagation::NULL_ID if no propagation::VarId is
   * associated with this node.
   */
  void setVarId(propagation::VarId varId);

  [[nodiscard]] const SearchDomain& constDomain() const noexcept;

  [[nodiscard]] SearchDomain& domain() noexcept;

  [[nodiscard]] bool isFixed() const noexcept;

  [[nodiscard]] Int val() const;

  void removeValue(Int);

  void removeValue(bool);

  [[nodiscard]] bool isIntVar() const noexcept;

  /**
   * @return if the bound range of the corresponding IntVar in solver is a
   * sub-set of SearchDomain _domain, then returns an empty vector, otherwise
   * the relative complement of varLb..varUb in SearchDomain is returned
   */
  [[nodiscard]] std::vector<DomainEntry> constrainedDomain(
      const propagation::SolverBase&);

  propagation::VarId postDomainConstraint(propagation::SolverBase&,
                                          std::vector<DomainEntry>&&);

  [[nodiscard]] std::pair<Int, Int> bounds() const;

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] const std::vector<InvariantNodeId>& inputTo() const noexcept;

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
  [[nodiscard]] InvariantNodeId outputOf() const noexcept;

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
};

}  // namespace atlantis::invariantgraph