#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

std::string to_string(const std::type_info& t, bool isVar) {
  if (t == typeid(fznparser::BoolVar)) {
    return "var bool";
  } else if (t == typeid(fznparser::IntVar)) {
    return "var int";
  } else if (t == typeid(fznparser::IntVarArray)) {
    if (isVar) {
      return "array[int] of var bool";
    } else {
      return "array[int] of bool";
    }
  } else if (t == typeid(fznparser::IntVarArray)) {
    if (isVar) {
      return "array[int] of var int";
    } else {
      return "array[int] of int";
    }
  } else if (t == typeid(fznparser::IntSet)) {
    return "set of int";
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

std::vector<Int> getFixedValues(const fznparser::IntVarArray& intVarArray) {
  std::vector<Int> values;
  values.reserve(intVarArray.size());
  for (size_t i = 0; i < intVarArray.size(); ++i) {
    if (std::holds_alternative<Int>(intVarArray.at(i))) {
      values.emplace_back(std::get<Int>(intVarArray.at(i)));
      continue;
    }
    const auto& var = std::get<std::reference_wrapper<const fznparser::IntVar>>(
                          intVarArray.at(i))
                          .get();

    if (var.isFixed()) {
      values.emplace_back(var.lowerBound());
    }
  }
  return values;
}

std::vector<bool> getFixedBoolValues(const InvariantGraph& invariantGraph,
                                     const std::vector<VarNodeId>& varNodeIds) {
  std::vector<bool> values;
  values.reserve(varNodeIds.size());
  for (VarNodeId varNodeId : varNodeIds) {
    const VarNode& varNode = invariantGraph.varNodeConst(varNodeId);
    if (varNode.isFixed()) {
      values.emplace_back(varNode.lowerBound() == 0);
    }
  }
  return values;
}

std::vector<bool> getFixedValues(const fznparser::BoolVarArray& boolVarArray) {
  std::vector<bool> values;
  values.reserve(boolVarArray.size());
  for (size_t i = 0; i < boolVarArray.size(); ++i) {
    if (std::holds_alternative<bool>(boolVarArray.at(i))) {
      values.emplace_back(std::get<bool>(boolVarArray.at(i)));
      continue;
    }
    const auto& var =
        std::get<std::reference_wrapper<const fznparser::BoolVar>>(
            boolVarArray.at(i))
            .get();

    if (var.isFixed()) {
      values.emplace_back(var.lowerBound());
    }
  }
  return values;
}

void verifyAllDifferent(const fznparser::IntVarArray& intVarArray) {
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

[[nodiscard]] bool violatesAllEqual(const fznparser::IntVarArray& intVarArray) {
  std::vector<Int> values = getFixedValues(intVarArray);
  for (size_t i = 1; i < values.size(); ++i) {
    if (values[i] != values[0]) {
      return true;
    }
  }
  return false;
}

VarNodeId createCountNode(FznInvariantGraph& invariantGraph,
                          const fznparser::IntVarArray& inputs,
                          const fznparser::IntArg& needle) {
  SearchDomain domain(0, static_cast<Int>(inputs.size()));

  VarNodeId countVarNodeId = invariantGraph.createVarNode(std::move(domain), true, true);

  if (needle.isFixed()) {
    invariantGraph.addInvariantNode(std::make_unique<IntCountNode>(
        invariantGraph.createVarNodes(inputs, false),
        needle.toParameter(), countVarNodeId));
  } else {
    invariantGraph.addInvariantNode(std::make_unique<VarIntCountNode>(
        invariantGraph.createVarNodes(inputs, false),
        invariantGraph.createVarNodeFromFzn(needle.var(), false),
        countVarNodeId));
  }
  return countVarNodeId;
}

VarNodeId createCountNode(FznInvariantGraph& invariantGraph,
                          const fznparser::IntVarArray& inputs,
                          const fznparser::IntArg& needle,
                          const fznparser::IntArg& count) {
  VarNodeId countVarNodeId = invariantGraph.createVarNodeFromFzn(count, true);

  if (needle.isFixed()) {
    invariantGraph.addInvariantNode(std::make_unique<IntCountNode>(
        invariantGraph.createVarNodes(inputs, false),
        needle.toParameter(), countVarNodeId));
  } else {
    invariantGraph.addInvariantNode(std::make_unique<VarIntCountNode>(
        invariantGraph.createVarNodes(inputs, false),
        invariantGraph.createVarNodeFromFzn(needle.var(), false),
        countVarNodeId));
  }
  return countVarNodeId;
}

std::pair<Int, Int> linBounds(const std::vector<Int>& coeffs,
                              const fznparser::BoolVarArray& vars) {
  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < coeffs.size(); ++i) {
    const auto& var = vars.at(i);
    Int v1, v2;
    if (std::holds_alternative<bool>(var)) {
      const Int val = std::get<bool>(var) ? 1 : 0;
      v1 = val * coeffs[i];
      v2 = val * coeffs[i];
    } else {
      const auto& varRef =
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(var).get();
      v1 = (varRef.lowerBound() ? 1 : 0) * coeffs[i];
      v2 = (varRef.upperBound() ? 1 : 0) * coeffs[i];
    }
    lb += std::min(v1, v2);
    ub += std::max(v1, v2);
  }
  return {lb, ub};
}

std::pair<Int, Int> linBounds(const std::vector<Int>& coeffs,
                              const fznparser::IntVarArray& vars) {
  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < coeffs.size(); ++i) {
    const auto& var = vars.at(i);
    Int v1, v2;
    if (std::holds_alternative<Int>(var)) {
      const Int val = std::get<Int>(var);
      v1 = val * coeffs[i];
      v2 = val * coeffs[i];
    } else {
      const auto& varRef =
          std::get<std::reference_wrapper<const fznparser::IntVar>>(var).get();
      v1 = varRef.lowerBound() * coeffs[i];
      v2 = varRef.upperBound() * coeffs[i];
    }
    lb += std::min(v1, v2);
    ub += std::max(v1, v2);
  }
  return {lb, ub};
}

}  // namespace atlantis::invariantgraph::fzn