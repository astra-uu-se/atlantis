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

class VariableDefiningNode;

/**
 * A variable in the invariant graph. Every variable is defined by a
 * VariableDefiningNode. The variable is possibly associated with a model
 * variable.
 */
class VariableNode {
 public:
  using FZNVariable =
      std::variant<fznparser::IntVariable, fznparser::BoolVariable>;
  using VariableMap = std::unordered_map<VariableNode*, VarId>;
  float SPARSE_MIN_DENSENESS{0.6};

 private:
  std::optional<FZNVariable> _variable;
  SearchDomain _domain;
  const bool _isIntVar;
  VarId _varId{NULL_ID};
  VarId _domainViolationId{NULL_ID};

  std::vector<VariableDefiningNode*> _inputFor;
  std::vector<VariableDefiningNode*> _staticInputFor;
  std::vector<VariableDefiningNode*> _dynamicInputFor;
  std::unordered_set<VariableDefiningNode*> _definingNodes;

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

  /**
   * @return The model VarId this node is associated with, or NULL_ID
   * if no VarId is associated with this node.
   */
  [[nodiscard]] VarId varId() const { return _varId; }

  /**
   * @return The model VarId this node is associated with, or NULL_ID
   * if no VarId is associated with this node.
   */
  void setVarId(VarId varId) {
    assert(_varId == NULL_ID);
    _varId = varId;
  }

  [[nodiscard]] SearchDomain& domain() noexcept { return _domain; }

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
  [[nodiscard]] const std::vector<VariableDefiningNode*>& inputFor()
      const noexcept {
    return _inputFor;
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] const std::vector<VariableDefiningNode*>& staticInputFor()
      const noexcept {
    return _staticInputFor;
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] const std::vector<VariableDefiningNode*>& dynamicInputFor()
      const noexcept {
    return _dynamicInputFor;
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] const std::unordered_set<VariableDefiningNode*>& definingNodes()
      const noexcept {
    return _definingNodes;
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] VariableDefiningNode* definedBy() const noexcept {
    if (_definingNodes.size() == 1) {
      return *_definingNodes.begin();
    }
    return nullptr;
  }

  /**
   * Indicate this variable node serves as an input to the given variable
   * defining node.
   *
   * @param node The variable defining node for which this is an input.
   */
  void markAsInputFor(VariableDefiningNode* listeningInvariant,
                      bool isStaticInput) {
    _inputFor.push_back(listeningInvariant);
    if (isStaticInput) {
      _staticInputFor.push_back(listeningInvariant);
    } else {
      _dynamicInputFor.push_back(listeningInvariant);
    }
  }

  /**
   * Indicate this variable node is no longer defined by the variable defining
   * node.
   *
   * @param node The variable defining node for which this is no longer defined.
   */
  void unmarkAsDefinedBy(VariableDefiningNode* definingInvariant) {
    _definingNodes.erase(definingInvariant);
  }

  /**
   * Indicate this variable node no longer serves as an input to the given
   * variable defining node.
   *
   * @param node The variable defining node for which this is no longer an
   * input.
   */
  void unmarkAsInputFor(VariableDefiningNode* listeningInvariant,
                        bool isStaticInput) {
    if (isStaticInput) {
      auto it = _staticInputFor.begin();
      while (it != _staticInputFor.end()) {
        if (*it == listeningInvariant) {
          it = _staticInputFor.erase(it);
        } else {
          it++;
        }
      }
    } else {
      auto it = _dynamicInputFor.begin();
      while (it != _dynamicInputFor.end()) {
        if (*it == listeningInvariant) {
          it = _dynamicInputFor.erase(it);
        } else {
          it++;
        }
      }
    }
  }

  /**
   * Indicate this variable node is defined by the given variable
   * defining node.
   *
   * @param node The variable defining node this is defined by.
   */
  void markDefinedBy(VariableDefiningNode* definingInvariant) {
    assert(definingInvariant != nullptr);
    _definingNodes.emplace(definingInvariant);
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