#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "constraints/lessEqual.hpp"
#include "constraints/lessThan.hpp"
#include "invariantgraph/softConstraintNode.hpp"
#include "invariants/count.hpp"
#include "invariants/countConst.hpp"
#include "views/equalConst.hpp"
#include "views/greaterEqualConst.hpp"
#include "views/lessEqualConst.hpp"
#include "views/notEqualConst.hpp"

namespace invariantgraph {
class CountGtNode : public SoftConstraintNode {
 private:
  const bool _yIsParameter;
  const Int _yParameter;
  const bool _cIsParameter;
  const Int _cParameter;
  VarId _intermediate{NULL_ID};

  explicit CountGtNode(std::vector<VariableNode*> x, VariableNode* y,
                       Int yParameter, VariableNode* c, Int cParameter,
                       VariableNode* r);

  explicit CountGtNode(std::vector<VariableNode*> x, VariableNode* y,
                       Int yParameter, VariableNode* c, Int cParameter,
                       bool shouldHold);

 public:
  explicit CountGtNode(std::vector<VariableNode*> x, VariableNode* y,
                       VariableNode* c, VariableNode* r);

  explicit CountGtNode(std::vector<VariableNode*> x, Int yParameter,
                       VariableNode* c, VariableNode* r);

  explicit CountGtNode(std::vector<VariableNode*> x, VariableNode* y,
                       Int cParameter, VariableNode* r);

  explicit CountGtNode(std::vector<VariableNode*> x, Int yParameter,
                       Int cParameter, VariableNode* r);

  explicit CountGtNode(std::vector<VariableNode*> x, VariableNode* y,
                       VariableNode* c, bool shouldHold);

  explicit CountGtNode(std::vector<VariableNode*> x, Int yParameter,
                       VariableNode* c, bool shouldHold);

  explicit CountGtNode(std::vector<VariableNode*> x, VariableNode* y,
                       Int cParameter, bool shouldHold);

  explicit CountGtNode(std::vector<VariableNode*> x, Int yParameter,
                       Int cParameter, bool shouldHold);

  static std::vector<std::pair<std::string_view, size_t>>
  acceptedNameNumArgPairs() {
    return std::vector<std::pair<std::string_view, size_t>>{
        {"fzn_count_gt", 3}, {"fzn_count_gt_reif", 4}};
  }

  static std::unique_ptr<CountGtNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;

  [[nodiscard]] VariableNode* yVarNode() const {
    return _yIsParameter
               ? nullptr
               : staticInputs().at(staticInputs().size() -
                                   (1 + static_cast<size_t>(!_cIsParameter)));
  }

  [[nodiscard]] VariableNode* cVarNode() const {
    return _cIsParameter ? nullptr : staticInputs().back();
  }
};
}  // namespace invariantgraph