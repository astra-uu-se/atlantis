#pragma once

#include <cassert>
#include <fznparser/ast.hpp>
#include <fznparser/model.hpp>
#include <unordered_map>
#include <vector>

#include "core/engine.hpp"
#include "invariantgraph/variableNode.hpp"

namespace invariantgraph {
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
      assert(definedVar->definingNodes().contains(this));
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

  virtual bool prune() { return false; }

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

  void replaceDefinedVariable(VariableNode* oldDefinedVar,
                              VariableNode* newDefinedVar) {
    // Replace all occurrences:
    for (size_t i = 0; i < _definedVariables.size(); ++i) {
      if (_definedVariables[i] == oldDefinedVar) {
        _definedVariables[i] = newDefinedVar;
      }
    }
    oldDefinedVar->unmarkAsDefinedBy(this);
    newDefinedVar->markDefinedBy(this);
  }

  void removeStaticInput(VariableNode* input) {
    // remove all occurrences:
    _staticInputs.erase(
        std::remove(_staticInputs.begin(), _staticInputs.end(), input),
        _staticInputs.end());
    input->unmarkAsInputFor(this, true);
  }

  void removeDefinedVariable(VariableNode* input) {
    // remove all occurrences:
    _definedVariables.erase(
        std::remove(_definedVariables.begin(), _definedVariables.end(), input),
        _definedVariables.end());
    input->unmarkAsDefinedBy(this);
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
}  // namespace invariantgraph