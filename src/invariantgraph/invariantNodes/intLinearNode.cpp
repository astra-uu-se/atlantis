#include "invariantgraph/invariantNodes/intLinearNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

IntLinearNode::IntLinearNode(std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars, VarNodeId output,
                             Int definingCoefficient, Int sum)
    : InvariantNode({output}, std::move(vars)),
      _coeffs(std::move(coeffs)),
      _definingCoefficient(definingCoefficient),
      _sum(sum) {}

std::unique_ptr<IntLinearNode> IntLinearNode::fromModelConstraint(
    const fznparser::Constraint& constraint, FznInvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  std::vector<Int> coeffs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0))
          .toParVector();

  const fznparser::IntVarArray& vars =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1));

  // The negative sum is the offset of the defined variable:
  auto sum =
      std::get<fznparser::IntArg>(constraint.arguments().at(2)).parameter();

  const std::optional<std::reference_wrapper<const fznparser::Var>>
      definedVarRef = constraint.definedVar();

  assert(definedVarRef.has_value());

  const fznparser::IntVar& definedVar =
      std::get<fznparser::IntVar>(definedVarRef.value().get());

  size_t definedVarIndex = vars.size();
  for (size_t i = 0; i < vars.size(); ++i) {
    if (!std::holds_alternative<Int>(vars.at(i)) &&
        std::get<std::reference_wrapper<const fznparser::IntVar>>(vars.at(i))
                .get()
                .identifier() == definedVar.identifier()) {
      definedVarIndex = i;
      break;
    }
  }

  assert(definedVarIndex < vars.size());

  Int definedVarCoeff = coeffs.at(definedVarIndex);

  // TODO: add a violation that is definedVar % coeffs[definedVarIndex]
  if (std::abs(definedVarCoeff) != 1) {
    throw std::runtime_error(
        "Cannot define variable with coefficient which is not +/-1");
  }

  auto coeffsIt = coeffs.begin();
  std::advance(coeffsIt, definedVarIndex);
  coeffs.erase(coeffsIt);

  std::vector<VarNodeId> addedVars = invariantGraph.createVarNodes(vars);
  auto varsIt = addedVars.begin();
  std::advance(varsIt, definedVarIndex);
  auto output = *varsIt;
  addedVars.erase(varsIt);

  return std::make_unique<IntLinearNode>(
      std::move(coeffs), std::move(addedVars), output, definedVarCoeff, sum);
}

void IntLinearNode::registerOutputVars(InvariantGraph& invariantGraph,
                                       propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() == 1 &&
      invariantGraph.varId(staticInputVarNodeIds().front()) !=
          propagation::NULL_ID) {
    if (_coeffs.front() == 1 && _sum == 0) {
      invariantGraph.varNode(outputVarNodeIds().front())
          .setVarId(invariantGraph.varId(staticInputVarNodeIds().front()));
      return;
    }

    if (_definingCoefficient == -1) {
      auto scalar = solver.makeIntView<propagation::ScalarView>(
          solver, invariantGraph.varId(staticInputVarNodeIds().front()),
          _coeffs.front());
      invariantGraph.varNode(outputVarNodeIds().front())
          .setVarId(solver.makeIntView<propagation::IntOffsetView>(
              solver, scalar, -_sum));
    } else {
      assert(_definingCoefficient == 1);
      auto scalar = solver.makeIntView<propagation::ScalarView>(
          solver, invariantGraph.varId(staticInputVarNodeIds().front()),
          -_coeffs.front());
      invariantGraph.varNode(outputVarNodeIds().front())
          .setVarId(solver.makeIntView<propagation::IntOffsetView>(
              solver, scalar, _sum));
    }

    return;
  }

  if (_intermediateVarId == propagation::NULL_ID) {
    _intermediateVarId = solver.makeIntVar(0, 0, 0);
    assert(invariantGraph.varId(outputVarNodeIds().front()) ==
           propagation::NULL_ID);

    auto offsetIntermediate = _intermediateVarId;
    if (_sum != 0) {
      offsetIntermediate = solver.makeIntView<propagation::IntOffsetView>(
          solver, _intermediateVarId, -_sum);
    }

    auto invertedIntermediate = offsetIntermediate;
    if (_definingCoefficient == 1) {
      invertedIntermediate = solver.makeIntView<propagation::ScalarView>(
          solver, offsetIntermediate, -1);
    }

    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(invertedIntermediate);
  }
}

void IntLinearNode::registerNode(InvariantGraph& invariantGraph,
                                 propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& node) { return invariantGraph.varId(node); });
  if (_intermediateVarId == propagation::NULL_ID) {
    assert(solverVars.size() == 1);
    return;
  }

  solver.makeInvariant<propagation::Linear>(solver, _intermediateVarId, _coeffs,
                                            solverVars);
}

}  // namespace atlantis::invariantgraph