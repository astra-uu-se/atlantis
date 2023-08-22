#pragma once

#include <cassert>
#include <fznparser/variables.hpp>
#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>

#include "core/engine.hpp"
#include "search/neighbourhoods/neighbourhood.hpp"
#include "search/searchVariable.hpp"
#include "utils/variant.hpp"
#include "views/inDomain.hpp"
#include "views/inSparseDomain.hpp"

namespace invariantgraph {

/**
 * The types that can be in an array of search variables.
 */
using MappableValue = std::variant<Int, bool, std::string_view>;

class InvariantNode;

/**
 * A variable in the invariant graph. Every variable is defined by a
 * InvariantNode. The variable is possibly associated with a model
 * variable.
 */
class VarNode {
 public:
  using FZNVariable = std::variant<fznparser::BoolVar, fznparser::IntVar>;
  using VariableMap = std::unordered_map<VarNodeId, VarId, VarNodeIdHash>;
  float SPARSE_MIN_DENSENESS{0.6};

 private:
  VarNodeId _varNodeId;
  std::optional<FZNVariable> _variable;
  SearchDomain _domain;
  const bool _isIntVar;
  VarId _varId{NULL_ID};
  VarId _domainViolationId{NULL_ID};

  std::vector<InvariantNodeId> _inputFor;
  std::vector<InvariantNodeId> _staticInputFor;
  std::vector<InvariantNodeId> _dynamicInputFor;
  std::unordered_set<InvariantNodeId, InvariantNodeIdHash> _definingNodes;

 public:
  /**
   * Construct a variable node which is associated with a model variable.
   *
   * @param variable The model variable this node is associated with.
   */
  explicit VarNode(VarNodeId, const FZNVariable& variable);

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
  [[nodiscard]] std::optional<FZNVariable> variable() const;

  [[nodiscard]] std::string_view identifier() const;

  /**
   * @return The model VarId this node is associated with, or NULL_ID
   * if no VarId is associated with this node.
   */
  [[nodiscard]] VarId varId() const;

  /**
   * @return The model VarId this node is associated with, or NULL_ID
   * if no VarId is associated with this node.
   */
  void setVarId(VarId varId) {
    assert(_varId == NULL_ID);
    _varId = varId;
  }

  [[nodiscard]] SearchDomain& domain() noexcept { return _domain; }

  [[nodiscard]] bool isFixed() const noexcept { return _domain.isFixed(); }

  [[nodiscard]] inline bool isIntVar() const noexcept { return _isIntVar; }

  /**
   * @return if the bound range of the corresponding IntVar in engine is a
   * sub-set of SearchDomain _domain, then returns an empty vector, otherwise
   * the relative complement of varLb..varUb in SearchDomain is returned
   */
  [[nodiscard]] std::vector<DomainEntry> constrainedDomain(const Engine&);

  VarId postDomainConstraint(Engine&, std::vector<DomainEntry>&&);

  [[nodiscard]] std::pair<Int, Int> bounds() const noexcept {
    return _domain.bounds();
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] const std::vector<InvariantNodeId>& inputFor() const noexcept {
    return _inputFor;
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] const std::vector<InvariantNodeId>& staticInputFor()
      const noexcept {
    return _staticInputFor;
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] const std::vector<InvariantNodeId>& dynamicInputFor()
      const noexcept {
    return _dynamicInputFor;
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] const std::unordered_set<InvariantNodeId, InvariantNodeIdHash>&
  definingNodes() const noexcept {
    return _definingNodes;
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] InvariantNodeId definedBy() const noexcept;

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
  void unmarkAsDefinedBy(InvariantNodeId definingInvariant);

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

}  // namespace invariantgraph