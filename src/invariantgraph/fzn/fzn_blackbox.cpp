#include "atlantis/invariantgraph/fzn/fzn_blackbox.hpp"

#include <memory>
#include <utility>

#include "./fznHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantNodes/blackboxNode.hpp"
#include "atlantis/misc/blackboxFunction.hpp"
#include "fznparser/annotation.hpp"
#include "fznparser/variables.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_blackbox(FznInvariantGraph& graph,
                  std::shared_ptr<blackbox::BlackBoxFn> blackboxFn,
                  const std::shared_ptr<fznparser::IntVarArray>& int_in,
                  const std::shared_ptr<fznparser::FloatVarArray>& float_in,
                  const std::shared_ptr<fznparser::IntVarArray>& int_out,
                  const std::shared_ptr<fznparser::FloatVarArray>& float_out) {
  if (float_in->size() > 0 || float_out->size() > 0) {
    throw FznArgumentException(
        "Blackbox constraint uses floating point variables, but this is "
        "not yet supported");
  }

  graph.addInvariantNode(std::make_shared<BlackBoxNode>(
      graph, std::move(blackboxFn), graph.retrieveVarNodes(int_in),
      graph.retrieveVarNodes(int_out)));
  return true;
}

bool fzn_blackbox(FznInvariantGraph& graph,
                  const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_blackbox") {
    return false;
  }

  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::FloatVarArray,
                                  true);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 2, fznparser::IntVarArray, true);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 3, fznparser::FloatVarArray,
                                  true);

  // The annotation arguments themselves are validated by
  // BlackBoxFn::fromAnnotation.
  const fznparser::Annotation* methodAnn = nullptr;
  for (const auto& ann : constraint.annotations()) {
    if (ann.identifier() == "blackbox_dll" ||
        ann.identifier() == "blackbox_exec") {
      if (methodAnn != nullptr) {
        throw FznArgumentException(
            "Constraint " + constraint.identifier() +
            " has multiple execution method annotations.");
      }
      methodAnn = &ann;
    }
  }
  if (methodAnn == nullptr) {
    throw FznArgumentException(
        "Constraint " + constraint.identifier() +
        " does not contain a execution method annotation.");
  }

  return fzn_blackbox(
      graph, blackbox::BlackBoxFn::fromAnnotation(*methodAnn),
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)),
      getArgArray<fznparser::FloatVarArray>(constraint.arguments().at(1)),
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(2)),
      getArgArray<fznparser::FloatVarArray>(constraint.arguments().at(3)));
}

}  // namespace atlantis::invariantgraph::fzn
