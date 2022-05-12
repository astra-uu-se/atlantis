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

/**
 * Serves as a marker for the invariant graph to start the application to the
 * propagation engine.
 */
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
      assert(_reifiedViolation->definingNodes().contains(this));
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