#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/softConstraintNode.hpp"

static std::vector<invariantgraph::VariableNode*> append(
    std::vector<invariantgraph::VariableNode*>& vars,
    invariantgraph::VariableNode* y) {
  if (y != nullptr) {
    vars.emplace_back(y);
  }
  return vars;
}

namespace invariantgraph {
class CountEqNode : public SoftConstraintNode {
 private:
  const bool _yIsParameter;
  const Int _yParameter;
  const bool _cIsParameter;
  const Int _cParameter;
  VarId _intermediate{NULL_ID};

  explicit CountEqNode(std::vector<VariableNode*> x, VariableNode* y,
                       Int yParameter, VariableNode* c, Int cParameter,
                       VariableNode* r)
      : SoftConstraintNode(c == nullptr ? std::vector<VariableNode*>{}
                                        : std::vector<VariableNode*>{c},
                           append(x, y), r),
        _yIsParameter(y == nullptr),
        _yParameter(yParameter),
        _cIsParameter(c == nullptr),
        _cParameter(cParameter) {}

  explicit CountEqNode(std::vector<VariableNode*> x, VariableNode* y,
                       Int yParameter, VariableNode* c, Int cParameter,
                       bool shouldHold)
      : SoftConstraintNode(c == nullptr ? std::vector<VariableNode*>{}
                                        : std::vector<VariableNode*>{c},
                           append(x, y), shouldHold),
        _yIsParameter(y == nullptr),
        _yParameter(yParameter),
        _cIsParameter(c == nullptr),
        _cParameter(cParameter) {}

 public:
  explicit CountEqNode(std::vector<VariableNode*> x, VariableNode* y,
                       Int cParameter, VariableNode* r)
      : CountEqNode(x, y, 0, nullptr, cParameter, r) {}

  explicit CountEqNode(std::vector<VariableNode*> x, Int yParameter,
                       Int cParameter, VariableNode* r)
      : CountEqNode(x, nullptr, yParameter, nullptr, cParameter, r) {}

  explicit CountEqNode(std::vector<VariableNode*> x, VariableNode* y,
                       VariableNode* c, bool shouldHold)
      : CountEqNode(x, y, 0, c, 0, shouldHold) {}

  explicit CountEqNode(std::vector<VariableNode*> x, Int yParameter,
                       VariableNode* c, bool shouldHold)
      : CountEqNode(x, nullptr, yParameter, c, 0, shouldHold) {}

  explicit CountEqNode(std::vector<VariableNode*> x, VariableNode* y,
                       Int cParameter, bool shouldHold)
      : CountEqNode(x, y, 0, nullptr, cParameter, shouldHold) {}

  explicit CountEqNode(std::vector<VariableNode*> x, Int yParameter,
                       Int cParameter, bool shouldHold)
      : CountEqNode(x, nullptr, yParameter, nullptr, cParameter, shouldHold) {}

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"fzn_count_eq", 3}, {"fzn_count_eq_reif", 4}};
  }

  static std::unique_ptr<CountEqNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  VariableNode* yVarNode() {
    if (_yIsParameter) {
      return nullptr;
    }
    return staticInputs().back();
  }

  VariableNode* cVarNode() {
    if (_cIsParameter) {
      return nullptr;
    }
    return definedVariables().back();
  }
};
}  // namespace invariantgraph