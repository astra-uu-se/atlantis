#pragma once

#include <cassert>
#include <fznparser/ast.hpp>
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
using MappableValue = std::variant<Int, bool, fznparser::Identifier>;

class InvariantNode;

/**
 * A variable in the invariant graph. Every variable is defined by a
 * InvariantNode. The variable is possibly associated with a model
 * variable.
 */
class VariableNode {
 public:
  using FZNVariable =
      std::variant<fznparser::IntVariable, fznparser::BoolVariable>;
  using VariableMap = std::unordered_map<VariableNode*, VarId>;
  float SPARSE_MIN_DENSENESS{0.6};

 private:
  std::unordered_set<InvariantNode*> _inputFor;
  std::unordered_set<InvariantNode*> _staticInputFor;
  std::unordered_set<InvariantNode*> _dynamicInputFor;
  std::unordered_map<InvariantNode*, VarId> _definingInvariants;
  std::optional<FZNVariable> _variable;
  SearchDomain _domain;
  const bool _isIntVar;
  VarId _domainViolationId{NULL_ID};
  VarId _varId{NULL_ID};

 public:
  /**
   * Construct a variable node which is associated with a model variable.
   *
   * @param variable The model variable this node is associated with.
   */
  explicit VariableNode(FZNVariable variable);

  /**
   * Construct a variable node which is not associated with a model variable.
   *
   * @param domain The domain of this variable.
   */
  explicit VariableNode(SearchDomain domain, bool isIntVar);

  /**
   * @return The model variable this node is associated with, or std::nullopt
   * if no model variable is associated with this node.
   */
  [[nodiscard]] std::optional<FZNVariable> variable() const {
    return _variable;
  }

  [[nodiscard]] SearchDomain& domain() noexcept { return _domain; }

  [[nodiscard]] inline bool isIntVar() const noexcept { return _isIntVar; }

  [[nodiscard]] inline bool isSearchVariable() const noexcept {
    return _definingInvariants.empty();
  }

  [[nodiscard]] inline bool isEvaluationVariable() const noexcept {
    return _inputFor.empty();
  }

  [[nodiscard]] inline bool isDefinedBy(
      InvariantNode* const invariantNode) const noexcept {
    return _definingInvariants.contains(invariantNode);
  }

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
  [[nodiscard]] const std::unordered_set<InvariantNode*>& inputFor()
      const noexcept {
    return _inputFor;
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] const std::unordered_set<InvariantNode*>& staticInputFor()
      const noexcept {
    return _staticInputFor;
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] const std::unordered_set<InvariantNode*>& dynamicInputFor()
      const noexcept {
    return _dynamicInputFor;
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] std::vector<InvariantNode*> definingNodes() const noexcept {
    std::vector<InvariantNode*> keys;
    keys.reserve(_definingInvariants.size());
    std::transform(_definingInvariants.begin(), _definingInvariants.end(),
                   keys.begin(), [&](const auto pair) { return pair.first; });
    return keys;
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] std::vector<VarId> varIds() const noexcept {
    std::vector<VarId> keys;
    keys.reserve(_definingInvariants.size());
    std::transform(_definingInvariants.begin(), _definingInvariants.end(),
                   keys.begin(), [&](const auto pair) { return pair.second; });
    return keys;
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] VarId varId(InvariantNode* const definingInvariant) noexcept {
    assert(_definingInvariants.contains(definingInvariant));
    return _definingInvariants.contains(definingInvariant)
               ? _definingInvariants[definingInvariant]
               : NULL_ID;
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] VarId inputVarId() const noexcept { return _varId; }

  /**
   * Indicate this variable node serves as an input to the given variable
   * defining node.
   *
   * @param node The variable defining node for which this is an input.
   */
  void markAsInputFor(InvariantNode* listeningInvariant, bool isStaticInput) {
    _inputFor.emplace(listeningInvariant);
    if (isStaticInput) {
      _staticInputFor.emplace(listeningInvariant);
    } else {
      _dynamicInputFor.emplace(listeningInvariant);
    }
  }

  /**
   * Indicate this variable node is no longer defined by the variable defining
   * node.
   *
   * @param node The variable defining node for which this is no longer defined.
   */
  void unmarkAsDefinedBy(InvariantNode* definingInvariant) {
    assert(varId(definingInvariant) == NULL_ID);
    _definingInvariants.erase(definingInvariant);
  }

  /**
   * Indicate this variable node no longer serves as an input to the given
   * variable defining node.
   *
   * @param node The variable defining node for which this is no longer an
   * input.
   */
  void unmarkAsInputFor(InvariantNode* listeningInvariant, bool isStaticInput) {
    _inputFor.erase(listeningInvariant);
    if (isStaticInput) {
      _staticInputFor.erase(listeningInvariant);
    } else {
      _staticInputFor.erase(listeningInvariant);
    }
  }

  inline void setVarId(VarId varId) { _varId = varId; }

  /**
   * Indicate this variable node is defined by the given variable
   * defining node.
   *
   * @param node The variable defining node this is defined by.
   */
  inline void setVarId(VarId varId, InvariantNode* definingInvariant) {
    assert(definingInvariant != nullptr);
    assert(varId != NULL_ID);
    assert(!_definingInvariants.contains(definingInvariant));
    if (_varId == NULL_ID) {
      setVarId(varId);
    }
    if (_definingInvariants.contains(definingInvariant)) {
      _definingInvariants[definingInvariant] = varId;
    } else {
      _definingInvariants.emplace(
          std::pair<InvariantNode*, VarId>(definingInvariant, varId));
    }
  }

  /**
   * Indicate this variable node is defined by the given variable
   * defining node.
   *
   * @param node The variable defining node this is defined by.
   */
  inline void markInputFor(InvariantNode* definingInvariant) {
    assert(definingInvariant != nullptr);
    if (!_definingInvariants.contains(definingInvariant)) {
      _definingInvariants.emplace(
          std::pair<InvariantNode*, VarId>(definingInvariant, NULL_ID));
    }
  }

  [[nodiscard]] inline bool isConstantValue() const noexcept {
    const auto [lb, ub] = _domain.bounds();
    return lb == ub;
  }

  /**
   * @return The constant value if this node represents a node, or std::nullopt
   * otherwise.
   */
  [[nodiscard]] std::optional<Int> constantValue() const noexcept {
    auto [lb, ub] = _domain.bounds();
    return lb == ub ? std::optional<Int>{lb} : std::optional<Int>{};
  }
};
}  // namespace invariantgraph