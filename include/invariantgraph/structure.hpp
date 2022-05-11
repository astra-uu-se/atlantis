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
  float SPARSE_MIN_DENSENESS{0.6};

 private:
  std::optional<FZNVariable> _variable;
  SearchDomain _domain;
  VarId _varId{NULL_ID};
  VarId _domainViolationId{NULL_ID};

  std::vector<VariableDefiningNode*> _inputFor;
  std::vector<VariableDefiningNode*> _staticInputFor;
  std::vector<VariableDefiningNode*> _dynamicInputFor;
  VariableDefiningNode* _definedBy{nullptr};

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
  explicit VariableNode(SearchDomain domain);

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
  [[nodiscard]] VariableDefiningNode* definedBy() const noexcept {
    return _definedBy;
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
    _definedBy = definingInvariant;
  }

  // A hack in order to steal the _inputs from the nested constraint.
  friend class ReifiedConstraint;
};

/**
 * A node in the invariant graph which defines a number of variables. This could
 * be an invariant, a soft constraint (which defines a violation), or a view.
 */
class VariableDefiningNode {
 private:
  std::vector<VariableNode*> _definedVariables;
  std::vector<VariableNode*> _staticInputs;
  std::vector<VariableNode*> _dynamicInputs;

 public:
  using VariableMap = std::unordered_map<VariableNode*, VarId>;

  explicit VariableDefiningNode(std::vector<VariableNode*> definedVariables,
                                std::vector<VariableNode*> staticInputs = {},
                                std::vector<VariableNode*> dynamicInputs = {})
      : _definedVariables(std::move(definedVariables)),
        _staticInputs(std::move(staticInputs)),
        _dynamicInputs(std::move(dynamicInputs)) {
    for (auto* const definedVar : _definedVariables) {
      markDefinedBy(definedVar, false);
      assert(definedVar->definedBy() == this);
    }
    for (auto* const staticInput : _staticInputs) {
      markAsStaticInput(staticInput, false);
    }
    for (auto* const dynamicInput : _dynamicInputs) {
      markAsDynamicInput(dynamicInput, false);
    }
  }

  virtual ~VariableDefiningNode() = default;

  [[nodiscard]] virtual bool isReified() const { return false; }

  /**
   * Creates as all the variables the node defines in @p engine.
   *
   * @param engine The engine with which to register the variables, constraints
   * and views.
   */
  virtual void createDefinedVariables(Engine& engine) = 0;

  /**
   * Registers the current node with the engine, as well as all the variables
   * it defines.
   *
   * Note: This method assumes it is called after all the inputs to this node
   * are already registered with the engine.
   *
   * @param engine The engine with which to register the variables, constraints
   * and views.
   */
  virtual void registerWithEngine(Engine& engine) = 0;

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
  [[nodiscard]] virtual VarId violationVarId() const { return NULL_ID; }

  [[nodiscard]] const std::vector<VariableNode*>& staticInputs()
      const noexcept {
    return _staticInputs;
  }

  [[nodiscard]] const std::vector<VariableNode*>& dynamicInputs()
      const noexcept {
    return _dynamicInputs;
  }

  void replaceStaticInput(VariableNode* oldInput, VariableNode* newInput) {
    // Replace all occurrences:
    for (size_t i = 0; i < _staticInputs.size(); ++i) {
      if (_staticInputs[i] == oldInput) {
        _staticInputs[i] = newInput;
      }
    }
    oldInput->unmarkAsInputFor(this, true);
    newInput->markAsInputFor(this, true);
  }

  void replaceDynamicInput(VariableNode* oldInput, VariableNode* newInput) {
    // Replace all occurrences:
    for (size_t i = 0; i < _dynamicInputs.size(); ++i) {
      if (_dynamicInputs[i] == oldInput) {
        _dynamicInputs[i] = newInput;
      }
    }
    oldInput->unmarkAsInputFor(this, false);
    newInput->markAsInputFor(this, false);
  }

  // A hack in order to steal the _inputs from the nested constraint.
  friend class ReifiedConstraint;

 protected:
  static inline VarId registerDefinedVariable(Engine& engine,
                                              VariableNode* variable,
                                              Int initialValue = 0) {
    if (variable->varId() == NULL_ID) {
      variable->setVarId(
          engine.makeIntVar(initialValue, initialValue, initialValue));
    }
    return variable->varId();
  }

  void markDefinedBy(VariableNode* node, bool registerHere = true) {
    node->markDefinedBy(this);

    if (registerHere) {
      _definedVariables.push_back(node);
    }
  }

  void addDefinedVariable(VariableNode* node) {
    _definedVariables.emplace_back(node);
    markDefinedBy(node, false);
  }

  void markAsStaticInput(VariableNode* node, bool registerHere = true) {
    node->markAsInputFor(this, true);

    if (registerHere) {
      _staticInputs.push_back(node);
    }
  }

  void markAsDynamicInput(VariableNode* node, bool registerHere = true) {
    node->markAsInputFor(this, false);

    if (registerHere) {
      _dynamicInputs.push_back(node);
    }
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

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  /**
   * Take the neighbourhood which is constructed in the registerWithEngine
   * call out of this instance. Note, this transfers ownership (as indicated
   * by the usage of unique_ptr).
   *
   * Calling this method before calling registerWithEngine will return a
   * nullptr. The same holds if this method is called multiple times. Only
   * the first call will return a neighbourhood instance.
   *
   * The reason this does not return a reference, is because we want to be
   * able to delete the entire invariant graph after it has been applied to
   * the propagation engine. If a reference was returned here, that would
   * leave the reference dangling.
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
  // Bounds will be recomputed by the engine.
  VarId _violationVarId{NULL_ID};
  VariableNode* _reifiedViolation;

 public:
  explicit SoftConstraintNode(std::vector<VariableNode*> staticInputs = {},
                              VariableNode* reifiedViolation = nullptr)
      : VariableDefiningNode(reifiedViolation == nullptr
                                 ? std::vector<VariableNode*>{}
                                 : std::vector<VariableNode*>{reifiedViolation},
                             std::move(staticInputs)),
        _reifiedViolation(reifiedViolation) {
    if (!isReified()) {
      assert(_reifiedViolation == nullptr);
      assert(definedVariables().size() == 0);
    } else {
      assert(_reifiedViolation != nullptr);
      assert(_reifiedViolation->definedBy() == this);
      assert(definedVariables().size() == 1);
      assert(definedVariables().front() == _reifiedViolation);
    }
  }

  ~SoftConstraintNode() override = default;

  [[nodiscard]] bool isReified() const override {
    return _reifiedViolation != nullptr;
  }

  [[nodiscard]] VarId violationVarId() const override {
    if (isReified()) {
      return _reifiedViolation->varId();
    }
    return _violationVarId;
  }

  inline VariableNode* reifiedViolation() { return _reifiedViolation; }

 protected:
  VarId setViolationVarId(VarId varId) {
    assert(violationVarId() == NULL_ID);
    if (isReified()) {
      _reifiedViolation->setVarId(varId);
    } else {
      _violationVarId = varId;
    }
    return violationVarId();
  }

  inline VarId registerViolation(Engine& engine, Int initialValue = 0) {
    if (violationVarId() == NULL_ID) {
      return setViolationVarId(
          engine.makeIntVar(initialValue, initialValue, initialValue));
    }
    return violationVarId();
  }
};

}  // namespace invariantgraph
