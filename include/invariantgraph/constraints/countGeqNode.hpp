#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/lessEqual.hpp"
#include "constraints/lessThan.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/count.hpp"
#include "invariants/countConst.hpp"
#include "views/equalView.hpp"
#include "views/greaterThanView.hpp"
#include "views/lessEqualView.hpp"
#include "views/notEqualView.hpp"

static std::vector<invariantgraph::VariableNode*> append(
    std::vector<invariantgraph::VariableNode*>& vars,
    invariantgraph::VariableNode* y, invariantgraph::VariableNode* c) {
  if (y != nullptr) {
    vars.emplace_back(y);
  }
  if (c != nullptr) {
    vars.emplace_back(c);
  }
  return vars;
}

namespace invariantgraph {
class CountGeqNode : public SoftConstraintNode {
 private:
  const bool _yIsParameter;
  const Int _yParameter;
  const bool _cIsParameter;
  const Int _cParameter;
  VarId _intermediate{NULL_ID};

  explicit CountGeqNode(std::vector<VariableNode*> x, VariableNode* y,
                        Int yParameter, VariableNode* c, Int cParameter,
                        VariableNode* r)
      : SoftConstraintNode(append(x, y, c), r),
        _yIsParameter(y == nullptr),
        _yParameter(yParameter),
        _cIsParameter(c == nullptr),
        _cParameter(cParameter) {}

  explicit CountGeqNode(std::vector<VariableNode*> x, VariableNode* y,
                        Int yParameter, VariableNode* c, Int cParameter,
                        bool shouldHold)
      : SoftConstraintNode(c == nullptr ? std::vector<VariableNode*>{}
                                        : std::vector<VariableNode*>{c},
                           append(x, y, c), shouldHold),
        _yIsParameter(y == nullptr),
        _yParameter(yParameter),
        _cIsParameter(c == nullptr),
        _cParameter(cParameter) {}

 public:
  explicit CountGeqNode(std::vector<VariableNode*> x, VariableNode* y,
                        VariableNode* c, VariableNode* r)
      : CountGeqNode(x, y, 0, c, 0, r) {}

  explicit CountGeqNode(std::vector<VariableNode*> x, Int yParameter,
                        VariableNode* c, VariableNode* r)
      : CountGeqNode(x, nullptr, yParameter, c, 0, r) {}

  explicit CountGeqNode(std::vector<VariableNode*> x, VariableNode* y,
                        Int cParameter, VariableNode* r)
      : CountGeqNode(x, y, 0, nullptr, cParameter, r) {}

  explicit CountGeqNode(std::vector<VariableNode*> x, Int yParameter,
                        Int cParameter, VariableNode* r)
      : CountGeqNode(x, nullptr, yParameter, nullptr, cParameter, r) {}

  explicit CountGeqNode(std::vector<VariableNode*> x, VariableNode* y,
                        VariableNode* c, bool shouldHold)
      : CountGeqNode(x, y, 0, c, 0, shouldHold) {}

  explicit CountGeqNode(std::vector<VariableNode*> x, Int yParameter,
                        VariableNode* c, bool shouldHold)
      : CountGeqNode(x, nullptr, yParameter, c, 0, shouldHold) {}

  explicit CountGeqNode(std::vector<VariableNode*> x, VariableNode* y,
                        Int cParameter, bool shouldHold)
      : CountGeqNode(x, y, 0, nullptr, cParameter, shouldHold) {}

  explicit CountGeqNode(std::vector<VariableNode*> x, Int yParameter,
                        Int cParameter, bool shouldHold)
      : CountGeqNode(x, nullptr, yParameter, nullptr, cParameter, shouldHold) {}

  static std::unique_ptr<CountGeqNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  VariableNode* yVarNode() {
    if (_yIsParameter) {
      return nullptr;
    }
    if (_cIsParameter) {
      return staticInputs().at(staticInputs().size() - 2);
    }
    return staticInputs().back();
  }

  VariableNode* cVarNode() {
    if (_cIsParameter) {
      return nullptr;
    }
    return staticInputs().back();
  }
};
}  // namespace invariantgraph