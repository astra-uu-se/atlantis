#pragma once

#include <cassert>
#include <fznparser/ast.hpp>
#include <vector>

#include "core/engine.hpp"
#include "invariantgraph/variableDefiningNode.hpp"
#include "invariantgraph/variableNode.hpp"

namespace invariantgraph {

/**
 * The types that can be in an array of search variables.
 */
using MappableValue = std::variant<Int, bool, fznparser::Identifier>;

static std::vector<invariantgraph::VariableNode*> combine(
    VariableNode* reifiedViolation, std::vector<VariableNode*> definedVars) {
  if (reifiedViolation == nullptr) {
    return definedVars;
  }
  definedVars.insert(definedVars.begin(), reifiedViolation);
  return definedVars;
}

/**
 * Serves as a marker for the invariant graph to start the application to the
 * propagation engine.
 */
class SoftConstraintNode : public VariableDefiningNode {
 private:
  // Bounds will be recomputed by the engine.
  VarId _violationVarId{NULL_ID};
  VariableNode* _reifiedViolation;

  // If the constraint is not reified, then this boolean indicates if the
  // constraint should hold or not:
  const bool _shouldHold;

  explicit SoftConstraintNode(std::vector<VariableNode*> definedVars,
                              std::vector<VariableNode*> staticInputs,
                              VariableNode* reifiedViolation, bool shouldHold)
      : VariableDefiningNode(combine(reifiedViolation, definedVars),
                             std::move(staticInputs)),
        _reifiedViolation(reifiedViolation),
        _shouldHold(shouldHold) {
    if (!isReified()) {
      assert(_reifiedViolation == nullptr);
    } else {
      assert(_reifiedViolation != nullptr);
      assert(_reifiedViolation->definingNodes().contains(this));
      assert(definedVariables().front() == _reifiedViolation);
    }
  }

 protected:
  inline bool shouldHold() const noexcept { return _shouldHold; }

 public:
  explicit SoftConstraintNode(std::vector<VariableNode*> definedVariables,
                              std::vector<VariableNode*> staticInputs,
                              VariableNode* reifiedViolation)
      : SoftConstraintNode(definedVariables, staticInputs, reifiedViolation,
                           true) {}

  explicit SoftConstraintNode(std::vector<VariableNode*> staticInputs,
                              VariableNode* reifiedViolation)
      : SoftConstraintNode({}, staticInputs, reifiedViolation, true) {}

  /**
   * @brief Construct a new Soft Constraint Node object
   *
   * @param staticInputs
   * @param shouldHold true if the constraint should hold, else false
   */
  explicit SoftConstraintNode(std::vector<VariableNode*> definedVariables,
                              std::vector<VariableNode*> staticInputs,
                              bool shouldHold)
      : SoftConstraintNode(definedVariables, staticInputs, nullptr,
                           shouldHold) {}

  explicit SoftConstraintNode(std::vector<VariableNode*> staticInputs,
                              bool shouldHold)
      : SoftConstraintNode({}, staticInputs, nullptr, shouldHold) {}

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