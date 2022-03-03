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
  explicit SoftConstraintNode(const std::vector<VariableNode*>& variables);
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
  virtual ~VariableNode() = default;

  virtual void registerWithEngine(Engine& engine,
                                  std::map<VariableNode*, VarId>& map);

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

class ViewNode : public VariableNode {
 private:
  VariableNode* _input;

 public:
  ViewNode(VariableNode* input,
           std::shared_ptr<fznparser::SearchVariable> output)
      : VariableNode(std::move(output)), _input(input) {}
  ~ViewNode() override = default;

  void registerWithEngine(Engine& engine,
                          std::map<VariableNode*, VarId>& map) override;

  [[nodiscard]] std::shared_ptr<fznparser::SearchVariable> input()
      const noexcept {
    return _input->variable();
  }

 protected:
  virtual std::shared_ptr<View> createView(Engine& engine,
                                           VarId variable) const = 0;
};

}  // namespace invariantgraph