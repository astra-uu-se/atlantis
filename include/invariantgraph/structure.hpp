#pragma once

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <variant>

#include "core/engine.hpp"
#include "fznparser/model.hpp"

namespace invariantgraph {

class VariableDefiningNode;

/**
 * A variable in the invariant graph. Every variable is defined by a
 * VariableDefiningNode. The variable is possibly associated with a model
 * variable.
 */
class VariableNode {
 public:
  struct Domain {
    Int lowerBound;
    Int upperBound;

    Domain(Int lb, Int ub) : lowerBound(lb), upperBound(ub) {}
  };

 private:
  std::shared_ptr<fznparser::SearchVariable> _variable;
  Domain _domain;

  std::vector<VariableDefiningNode*> _inputFor;

 public:
  /**
   * Construct a variable node which is associated with a model variable.
   *
   * @param variable The model variable this node is associated with.
   */
  explicit VariableNode(std::shared_ptr<fznparser::SearchVariable> variable)
      : _variable(std::move(variable)),
        _domain{_variable->domain()->lowerBound(),
                _variable->domain()->upperBound()} {}

  /**
   * Construct a variable node which is not associated with a model variable.
   *
   * @param domain The domain of this variable.
   */
  explicit VariableNode(Domain domain) : _variable(nullptr), _domain(domain) {}

  /**
   * @return The model variable this node is associated with. If the node is not
   * associated with a model variable, this returns a @p nullptr.
   */
  [[nodiscard]] std::shared_ptr<fznparser::SearchVariable> variable() const {
    return _variable;
  }

  void imposeDomain(Domain domain) { _domain = domain; }

  [[nodiscard]] const Domain& domain() const noexcept { return _domain; }

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
  virtual void registerWithEngine(
      Engine& engine, std::map<VariableNode*, VarId>& variableMap) = 0;

  /**
   * @return The variable nodes defined by this node.
   */
  [[nodiscard]] const std::vector<VariableNode*>& definedVariables() const noexcept {
    return _definedVariables;
  }

  /**
   * @return The violation variable of this variable defining node. Only
   * applicable if the current node is a soft constraint. If this node does not
   * define a violation variable, this method returns @p nullptr.
   */
  [[nodiscard]] virtual VariableNode* violation() { return nullptr; }

 protected:
  inline VarId registerDefinedVariable(
      Engine& engine, std::map<VariableNode*, VarId>& variableMap,
      VariableNode* variable) {
    const auto& [lb, ub] = variable->domain();
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
 public:
  explicit ImplicitConstraintNode(std::vector<VariableNode*> definedVariables)
      : VariableDefiningNode(std::move(definedVariables)) {}
  ~ImplicitConstraintNode() override = default;

  void registerWithEngine(
      Engine& engine, std::map<VariableNode*, VarId>& variableMap) override;
};

class SoftConstraintNode : public VariableDefiningNode {
 private:
  VariableNode _violation;

 public:
  explicit SoftConstraintNode(const std::function<Int()>& violationUb,
                              const std::vector<VariableNode*>& inputs = {})
      : VariableDefiningNode({&_violation}, inputs),
        _violation(VariableNode::Domain{0, violationUb()}) {}

  ~SoftConstraintNode() override = default;

  [[nodiscard]] VariableNode* violation() override { return &_violation; }

 protected:
  inline VarId registerViolation(Engine& engine,
                                 std::map<VariableNode*, VarId>& variableMap) {
    return registerDefinedVariable(engine, variableMap, violation());
  }
};

}  // namespace invariantgraph