#include "./fznHelper.hpp"

#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/varIntCountNode.hpp"

namespace atlantis::invariantgraph::fzn {

std::string to_string(const std::type_info& t, bool isVar) {
  // Arg:
  if (t == typeid(fznparser::BoolArg) ||
      t == typeid(std::shared_ptr<fznparser::BoolArg>) ||
      t == typeid(std::shared_ptr<const fznparser::BoolArg>)) {
    if (isVar) {
      return "{var bool, bool}";
    } else {
      return "bool";
    }
  } else if (t == typeid(fznparser::IntArg) ||
             t == typeid(std::shared_ptr<fznparser::IntArg>) ||
             t == typeid(std::shared_ptr<const fznparser::IntArg>)) {
    if (isVar) {
      return "{var int, int}";
    } else {
      return "int";
    }
  } else if (t == typeid(fznparser::FloatArg) ||
             t == typeid(std::shared_ptr<fznparser::FloatArg>) ||
             t == typeid(std::shared_ptr<const fznparser::FloatArg>)) {
    if (isVar) {
      return "{var float, float}";
    } else {
      return "float";
    }
  } else if (t == typeid(fznparser::IntSetArg) ||
             t == typeid(std::shared_ptr<fznparser::IntSetArg>) ||
             t == typeid(std::shared_ptr<const fznparser::IntSetArg>)) {
    if (isVar) {
      return "{var set of int, set of int}";
    } else {
      return "set of int";
    }
  } else if (t == typeid(fznparser::FloatSet) ||
             t == typeid(std::shared_ptr<fznparser::FloatSet>) ||
             t == typeid(std::shared_ptr<const fznparser::FloatSet>)) {
    return "set of float";
  } else if (t == typeid(fznparser::BoolVarArray) ||
             t == typeid(std::shared_ptr<fznparser::BoolVarArray>) ||
             t == typeid(std::shared_ptr<const fznparser::BoolVarArray>)) {
    if (isVar) {
      return "array[int] of var bool";
    } else {
      return "array[int] of bool";
    }
  } else if (t == typeid(fznparser::IntVarArray) ||
             t == typeid(std::shared_ptr<fznparser::IntVarArray>) ||
             t == typeid(std::shared_ptr<const fznparser::IntVarArray>)) {
    if (isVar) {
      return "array[int] of var int";
    } else {
      return "array[int] of int";
    }
  } else if (t == typeid(fznparser::FloatVarArray) ||
             t == typeid(std::shared_ptr<fznparser::FloatVarArray>) ||
             t == typeid(std::shared_ptr<const fznparser::FloatVarArray>)) {
    if (isVar) {
      return "array[int] of var float";
    } else {
      return "array[int] of float";
    }
  } else if (t == typeid(fznparser::SetVarArray) ||
             t == typeid(std::shared_ptr<fznparser::SetVarArray>) ||
             t == typeid(std::shared_ptr<const fznparser::SetVarArray>)) {
    if (isVar) {
      return "array[int] of set of var int";
    } else {
      return "array[int] of set of int";
    }
  } else if (t == typeid(fznparser::FloatSetArray) ||
             t == typeid(std::shared_ptr<fznparser::FloatSetArray>) ||
             t == typeid(std::shared_ptr<const fznparser::FloatSetArray>)) {
    return "array[int] of set of float";
  } else if (t == typeid(fznparser::IntSet) ||
             t == typeid(std::shared_ptr<fznparser::IntSet>) ||
             t == typeid(std::shared_ptr<const fznparser::IntSet>)) {
    return "set of int";
  } else if (t == typeid(fznparser::BoolVar) ||
             t == typeid(std::shared_ptr<fznparser::BoolVar>) ||
             t == typeid(std::shared_ptr<const fznparser::BoolVar>)) {  // Vars:
    return "var bool";
  } else if (t == typeid(fznparser::IntVar) ||
             t == typeid(std::shared_ptr<fznparser::IntVar>) ||
             t == typeid(std::shared_ptr<const fznparser::IntVar>)) {
    return "var int";
  } else if (t == typeid(fznparser::FloatVar) ||
             t == typeid(std::shared_ptr<fznparser::FloatVar>) ||
             t == typeid(std::shared_ptr<const fznparser::FloatVar>)) {
    return "var float";
  } else if (t == typeid(fznparser::SetVar) ||
             t == typeid(std::shared_ptr<fznparser::SetVar>) ||
             t == typeid(std::shared_ptr<const fznparser::SetVar>)) {
    return "set of var int";
  } else if (t == typeid(bool)) {
    return "bool";
  } else if (t == typeid(Int)) {
    return "int";
  } else if (t == typeid(double)) {
    return "float";
  } else if (t == typeid(fznparser::IntSet) ||
             t == typeid(std::shared_ptr<fznparser::IntSet>)) {
    return "set of int";
  } else if (t == typeid(fznparser::FloatSet) ||
             t == typeid(std::shared_ptr<fznparser::FloatSet>)) {
    return "set of float";
  }
  return "[unknown type]";
}

bool hasSuffix(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() &&
         0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

// "_reif".size() == 5
bool constraintIdentifierIsReified(const fznparser::Constraint& constraint) {
  return hasSuffix(constraint.identifier(), "_reif");
}

void verifyNumArguments(const fznparser::Constraint& constraint, size_t size) {
  if (constraint.arguments().size() != size) {
    throw FznArgumentException("Constraint " + constraint.identifier() +
                               " expects " + std::to_string(size) +
                               " arguments().");
  }
}

std::vector<Int> getFixedValues(
    const std::shared_ptr<fznparser::IntVarArray>& intVarArray) {
  std::vector<Int> values;
  values.reserve(intVarArray->size());
  for (size_t i = 0; i < intVarArray->size(); ++i) {
    if (std::holds_alternative<Int>(intVarArray->at(i))) {
      values.emplace_back(std::get<Int>(intVarArray->at(i)));
      continue;
    }
    const auto var =
        std::get<std::shared_ptr<const fznparser::IntVar>>(intVarArray->at(i));

    if (var->isFixed()) {
      values.emplace_back(var->lowerBound());
    }
  }
  return values;
}

std::vector<bool> getFixedBoolValues(const IInvariantGraph& graph,
                                     const std::vector<VarNodeId>& varNodeIds) {
  std::vector<bool> values;
  values.reserve(varNodeIds.size());
  for (VarNodeId varNodeId : varNodeIds) {
    const VarNode& varNode = graph.varNodeConst(varNodeId);
    if (varNode.isFixed()) {
      values.emplace_back(varNode.lowerBound() == 0);
    }
  }
  return values;
}

std::vector<bool> getFixedValues(
    const std::shared_ptr<fznparser::BoolVarArray>& boolVarArray) {
  std::vector<bool> values;
  values.reserve(boolVarArray->size());
  for (size_t i = 0; i < boolVarArray->size(); ++i) {
    if (std::holds_alternative<bool>(boolVarArray->at(i))) {
      values.emplace_back(std::get<bool>(boolVarArray->at(i)));
      continue;
    }
    const auto var = std::get<std::shared_ptr<const fznparser::BoolVar>>(
        boolVarArray->at(i));

    if (var->isFixed()) {
      values.emplace_back(var->lowerBound());
    }
  }
  return values;
}

std::vector<VarNodeId> retrieveUnfixedVarNodeIds(
    FznInvariantGraph& graph,
    const std::shared_ptr<fznparser::IntVarArray>& intVarArray) {
  std::vector<VarNodeId> vars;
  vars.reserve(intVarArray->size());
  for (size_t i = 0; i < intVarArray->size(); ++i) {
    if (std::holds_alternative<Int>(intVarArray->at(i))) {
      continue;
    }
    const auto var =
        std::get<std::shared_ptr<const fznparser::IntVar>>(intVarArray->at(i));

    if (!var->isFixed()) {
      vars.emplace_back(graph.retrieveVarNode(var));
    }
  }
  return vars;
}

std::vector<VarNodeId> retrieveUnfixedVarNodeIds(
    FznInvariantGraph& graph,
    const std::shared_ptr<fznparser::BoolVarArray>& boolVarArray) {
  std::vector<VarNodeId> vars;
  vars.reserve(boolVarArray->size());
  for (size_t i = 0; i < boolVarArray->size(); ++i) {
    if (std::holds_alternative<bool>(boolVarArray->at(i))) {
      continue;
    }
    const auto var = std::get<std::shared_ptr<const fznparser::BoolVar>>(
        boolVarArray->at(i));

    if (!var->isFixed()) {
      vars.emplace_back(graph.retrieveVarNode(var));
    }
  }
  return vars;
}

std::vector<VarNodeId> getUnfixedVarNodeIds(
    const IInvariantGraph& graph, const std::vector<VarNodeId>& varNodeIds) {
  std::vector<VarNodeId> unfixed;
  unfixed.reserve(varNodeIds.size());
  for (VarNodeId varNodeId : varNodeIds) {
    if (!graph.varNodeConst(varNodeId).isFixed()) {
      unfixed.emplace_back(varNodeId);
    }
  }
  return unfixed;
}

void verifyAllDifferent(
    const std::shared_ptr<fznparser::IntVarArray>& intVarArray) {
  std::vector<Int> values = getFixedValues(intVarArray);
  std::unordered_set<Int> seenValues;
  seenValues.reserve(values.size());
  for (const Int val : values) {
    if (seenValues.contains(val)) {
      throw FznArgumentException(
          "The argument of constraint fzn_all_different has parameters with "
          "duplicate values.");
    }
    seenValues.insert(val);
  }
}

[[nodiscard]] bool violatesAllEqual(
    const std::shared_ptr<fznparser::IntVarArray>& intVarArray) {
  std::vector<Int> values = getFixedValues(intVarArray);
  for (size_t i = 1; i < values.size(); ++i) {
    if (values[i] != values[0]) {
      return true;
    }
  }
  return false;
}

VarNodeId createCountNode(FznInvariantGraph& graph,
                          const std::shared_ptr<fznparser::IntVarArray>& inputs,
                          const fznparser::IntArg& needle) {
  VarNodeId countVarNodeId = graph.retrieveIntVarNode(
      SearchDomain(0, static_cast<Int>(inputs->size())));

  if (needle.isFixed()) {
    graph.addInvariantNode(
        std::make_shared<IntCountNode>(graph, graph.retrieveVarNodes(inputs),
                                       needle.toParameter(), countVarNodeId));
  } else {
    graph.addInvariantNode(std::make_shared<VarIntCountNode>(
        graph, graph.retrieveVarNodes(inputs),
        graph.retrieveVarNode(needle.var()), countVarNodeId));
  }
  return countVarNodeId;
}

VarNodeId createCountNode(FznInvariantGraph& graph,
                          const std::shared_ptr<fznparser::IntVarArray>& inputs,
                          const fznparser::IntArg& needle,
                          const fznparser::IntArg& count) {
  VarNodeId countVarNodeId = graph.retrieveVarNode(count);

  if (needle.isFixed()) {
    graph.addInvariantNode(
        std::make_shared<IntCountNode>(graph, graph.retrieveVarNodes(inputs),
                                       needle.toParameter(), countVarNodeId));
  } else {
    graph.addInvariantNode(std::make_shared<VarIntCountNode>(
        graph, graph.retrieveVarNodes(inputs),
        graph.retrieveVarNode(needle.var()), countVarNodeId));
  }
  return countVarNodeId;
}

void invertCoeffs(std::vector<Int>& coeffs) {
  for (size_t i = 0; i < coeffs.size(); ++i) {
    coeffs[i] = -coeffs[i];
  }
}

std::pair<Int, Int> linBounds(
    const std::vector<Int>& coeffs,
    const std::shared_ptr<fznparser::BoolVarArray>& vars) {
  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < coeffs.size(); ++i) {
    const auto& var = vars->at(i);
    Int v1, v2;
    if (std::holds_alternative<bool>(var)) {
      const Int val = std::get<bool>(var) ? 1 : 0;
      v1 = val * coeffs[i];
      v2 = val * coeffs[i];
    } else {
      const auto& varPtr =
          std::get<std::shared_ptr<const fznparser::BoolVar>>(var);
      v1 = (varPtr->lowerBound() ? 1 : 0) * coeffs[i];
      v2 = (varPtr->upperBound() ? 1 : 0) * coeffs[i];
    }
    lb += std::min(v1, v2);
    ub += std::max(v1, v2);
  }
  return {lb, ub};
}

std::pair<Int, Int> linBounds(
    const std::vector<Int>& coeffs,
    const std::shared_ptr<fznparser::IntVarArray>& vars) {
  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < coeffs.size(); ++i) {
    const auto& var = vars->at(i);
    Int v1, v2;
    if (std::holds_alternative<Int>(var)) {
      const Int val = std::get<Int>(var);
      v1 = val * coeffs[i];
      v2 = val * coeffs[i];
    } else {
      const auto& varPtr =
          std::get<std::shared_ptr<const fznparser::IntVar>>(var);
      v1 = varPtr->lowerBound() * coeffs[i];
      v2 = varPtr->upperBound() * coeffs[i];
    }
    lb += std::min(v1, v2);
    ub += std::max(v1, v2);
  }
  return {lb, ub};
}

std::pair<Int, Int> linBounds(FznInvariantGraph& invariantgraph,
                              const std::vector<Int>& coeffs,
                              const std::vector<VarNodeId>& varNodeIds) {
  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < coeffs.size(); ++i) {
    const auto& varNode = invariantgraph.varNode(varNodeIds.at(i));
    int v1 = coeffs.at(i) * (varNode.isIntVar()
                                 ? varNode.lowerBound()
                                 : static_cast<Int>(!varNode.inDomain(false)));
    int v2 = coeffs.at(i) * (varNode.isIntVar()
                                 ? varNode.upperBound()
                                 : static_cast<Int>(varNode.inDomain(true)));
    lb += std::min(v1, v2);
    ub += std::max(v1, v2);
  }
  return {lb, ub};
}

}  // namespace atlantis::invariantgraph::fzn