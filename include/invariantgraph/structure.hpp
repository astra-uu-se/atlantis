#pragma once

#include <cassert>
#include <fznparser/ast.hpp>
#include <optional>
#include <unordered_map>
#include <vector>

#include "constraints/inDomain.hpp"
#include "constraints/inSparseDomain.hpp"
#include "core/engine.hpp"
#include "search/neighbourhoods/neighbourhood.hpp"
#include "search/searchVariable.hpp"
#include "utils/variant.hpp"

namespace invariantgraph {

class VariableDefiningNode;

/**
 * The types that can be in an array of search variables.
 */
using MappableValue = std::variant<Int, bool, fznparser::Identifier>;

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
  static const size_t SPARSE_MAX_INTERVAL = 1000;
  float SPARSE_MIN_DENSENESS{0.1};

 private:
  std::optional<FZNVariable> _variable;
  SearchDomain _domain;
  VarId _domainViolationId{NULL_ID};

  std::vector<VariableDefiningNode*> _inputFor;

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
  explicit VariableNode(SearchDomain domain)
      : _variable(std::nullopt), _domain(std::move(domain)) {}

  /**
   * @return The model variable this node is associated with, or std::nullopt
   * if no model variable is associated with this node.
   */
  [[nodiscard]] std::optional<FZNVariable> variable() const {
    return _variable;
  }

  /**
   * @return if the bound range of the corresponding IntVar in engine is a
   * sub-set of SearchDomain _domain, then returns an empty vector, otherwise
   * the relative complement of varLb..varUb in SearchDomain is returned
   */
  [[nodiscard]] std::vector<DomainEntry> constrainedDomain(
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

  VarId postDomainConstraint(Engine& engine, const VariableMap& variableMap,
                             std::vector<DomainEntry>&& domain) {
    if (domain.size() == 0 || _domainViolationId != NULL_ID) {
      return _domainViolationId;
    }
    _domainViolationId = engine.makeIntVar(0, 0, 0);
    const size_t interval =
        domain.back().upperBound - domain.front().lowerBound + 1;
    const float denseness = (float)domain.size() / (float)interval;
    if (interval <= SPARSE_MAX_INTERVAL && SPARSE_MIN_DENSENESS <= denseness) {
      engine.makeConstraint<InSparseDomain>(
          _domainViolationId, variableMap.at(this), std::move(domain));
    } else {
      engine.makeConstraint<InDomain>(_domainViolationId, variableMap.at(this),
                                      std::move(domain));
    }

    return _domainViolationId;
  }

  void imposeDomain(SearchDomain domain) { _domain = std::move(domain); }

  [[nodiscard]] SearchDomain& domain() noexcept { return _domain; }

  [[nodiscard]] std::pair<Int, Int> bounds() const noexcept {
    return std::visit<std::pair<Int, Int>>(
        [&](auto& domain) {
          return std::make_pair(domain.lowerBound(), domain.upperBound());
        },
        _domain);
  }

  /**
   * @return The variable defining nodes for which this node is an input.
   */
  [[nodiscard]] const std::vector<VariableDefiningNode*>& inputFor()
      const noexcept {
    return _inputFor;
  }

  /**
   * Indicate this variable node serves as an input to the given variable
   * defining node.
   *
   * @param node The variable defining node for which this is an input.
   */
  void markAsInputFor(VariableDefiningNode* node) { _inputFor.push_back(node); }
};

/**
 * A node in the invariant graph which defines a number of variables. This could
 * be an invariant, a soft constraint (which defines a violation), or a view.
 */
class VariableDefiningNode {
 private:
  std::vector<VariableNode*> _definedVariables;

 public:
  using VariableMap = std::unordered_map<VariableNode*, VarId>;

  explicit VariableDefiningNode(std::vector<VariableNode*> definedVariables,
                                const std::vector<VariableNode*>& inputs = {})
      : _definedVariables(std::move(definedVariables)) {
    for (const auto& input : inputs) {
      input->markAsInputFor(static_cast<VariableDefiningNode*>(this));
    }
  }

  virtual ~VariableDefiningNode() = default;

  /**
   * Registers the current node with the engine, as well as all the variables
   * it defines. The @p variableMap is modified to contain the variables defined
   * by this node with their corresponding VarId's.
   *
   * Note: This method assumes it is called after all the inputs to this node
   * are already registered with the engine. This means the node should be able
   * to look up all the VarId's for its inputs in @p variableMap. Therefore,
   * nodes should be registered with the engine in a breadth-first manner.
   *
   * @param engine The engine with which to register the variables, constraints
   * and views.
   * @param variableMap A map of variable nodes to VarIds.
   */
  virtual void registerWithEngine(Engine& engine, VariableMap& variableMap) = 0;

  /**
   * @return The variable nodes defined by this node.
   */
  [[nodiscard]] const std::vector<VariableNode*>& definedVariables()
      const noexcept {
    return _definedVariables;
  }

  /**
   * @return The violation variable of this variable defining node. Only
   * applicable if the current node is a soft constraint. If this node does not
   * define a violation variable, this method returns @p nullptr.
   */
  [[nodiscard]] virtual VariableNode* violation() { return nullptr; }

 protected:
  static inline VarId registerDefinedVariable(Engine& engine,
                                              VariableMap& variableMap,
                                              VariableNode* variable) {
    const auto& [lb, ub] = variable->bounds();
    auto varId = engine.makeIntVar(lb, lb, ub);

    variableMap.emplace(variable, varId);

    return varId;
  }
};

/**
 * Serves as a marker for the invariant graph to start the application to the
 * propagation engine.
 */
class ImplicitConstraintNode : public VariableDefiningNode {
 private:
  search::neighbourhoods::Neighbourhood* _neighbourhood{nullptr};

 public:
  explicit ImplicitConstraintNode(std::vector<VariableNode*> definedVariables)
      : VariableDefiningNode(std::move(definedVariables)) {}
  ~ImplicitConstraintNode() override { delete _neighbourhood; }

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  /**
   * Take the neighbourhood which is constructed in the registerWithEngine
   * call out of this instance. Note, this transfers ownership (as indicated
   * by the usage of unique_ptr).
   *
   * Calling this method before calling registerWithEngine will return a
   * nullptr. The same holds if this method is called multiple times. Only the
   * first call will return a neighbourhood instance.
   *
   * The reason this does not return a reference, is because we want to be able
   * to delete the entire invariant graph after it has been applied to the
   * propagation engine. If a reference was returned here, that would leave the
   * reference dangling.
   *
   * @return The neighbourhood corresponding to this implicit constraint.
   */
  [[nodiscard]] std::unique_ptr<search::neighbourhoods::Neighbourhood>
  takeNeighbourhood() noexcept {
    auto ptr =
        std::unique_ptr<search::neighbourhoods::Neighbourhood>(_neighbourhood);
    _neighbourhood = nullptr;
    return ptr;
  }

 protected:
  virtual search::neighbourhoods::Neighbourhood* createNeighbourhood(
      Engine& engine, std::vector<search::SearchVariable> variables) = 0;
};

class SoftConstraintNode : public VariableDefiningNode {
 private:
  VariableNode _violation;

 public:
  explicit SoftConstraintNode(const std::function<Int()>& violationUb,
                              const std::vector<VariableNode*>& inputs = {})
      : VariableDefiningNode({&_violation}, inputs),
        _violation(IntervalDomain{0, violationUb()}) {}

  ~SoftConstraintNode() override = default;

  [[nodiscard]] VariableNode* violation() override { return &_violation; }

 protected:
  inline VarId registerViolation(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
    return registerDefinedVariable(engine, variableMap, violation());
  }
};

}  // namespace invariantgraph
