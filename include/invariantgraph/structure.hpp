#pragma once

#include <functional>
#include <memory>
#include <set>
#include <utility>
#include <variant>

#include "core/engine.hpp"
#include "fznparser/model.hpp"

namespace invariantgraph {
class VariableNode;
class InvariantNode {
 private:
  VariableNode* _output;

 public:
  explicit InvariantNode(VariableNode* output) : _output(output) {}
  virtual ~InvariantNode() = default;

  virtual void registerWithEngine(
      Engine& engine,
      std::function<VarId(VariableNode*)> variableMapper) const = 0;

  [[nodiscard]] VariableNode* output() const { return _output; }
};

class SoftConstraintNode {
 public:
  virtual ~SoftConstraintNode() = default;

  virtual VarId registerWithEngine(
      Engine& engine,
      std::function<VarId(VariableNode*)> variableMapper) const = 0;
};

class ImplicitConstraintNode {
 private:
  std::vector<VariableNode*> _definingVariables;

 public:
  virtual ~ImplicitConstraintNode() = default;

  [[nodiscard]] const std::vector<VariableNode*>& definingVariables() const {
    return _definingVariables;
  }
};

class VariableNode {
 private:
  std::shared_ptr<fznparser::SearchVariable> _variable;
  std::set<SoftConstraintNode*> _softConstraints;
  std::optional<InvariantNode*> _definingInvariant;
  Int _offset = 0;

 public:
  explicit VariableNode(std::shared_ptr<fznparser::SearchVariable> variable)
      : _variable(std::move(variable)) {}

  [[nodiscard]] std::shared_ptr<fznparser::SearchVariable> variable() const {
    return _variable;
  }

  void addSoftConstraint(SoftConstraintNode* node) {
    _softConstraints.emplace(node);
  }

  [[nodiscard]] const std::set<SoftConstraintNode*>& softConstraints() const {
    return _softConstraints;
  }

  void definedByInvariant(InvariantNode* node) { _definingInvariant = node; }

  [[nodiscard]] std::optional<InvariantNode*> definingInvariant() const {
    return _definingInvariant;
  }

  [[nodiscard]] bool isFunctionallyDefined() const {
    return definingInvariant().has_value();
  }

  [[nodiscard]] Int offset() const { return _offset; }

  void setOffset(Int offset) { _offset = offset; }
};

}  // namespace invariantgraph