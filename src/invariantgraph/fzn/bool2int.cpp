#include "atlantis/invariantgraph/fzn/bool2int.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/views/bool2IntNode.hpp"
#include "atlantis/invariantgraph/views/int2BoolNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool2int(FznInvariantGraph& invariantGraph,
              const fznparser::BoolArg& boolArg,
              const fznparser::IntArg& intArg) {
  invariantGraph.addInvariantNode(
      std::make_unique<Bool2IntNode>(invariantGraph.retrieveVarNode(boolArg),
                                     invariantGraph.retrieveVarNode(intArg)));
  return true;
}

bool int2bool(FznInvariantGraph& invariantGraph,
              const fznparser::BoolArg& boolArg,
              const fznparser::IntArg& intArg) {
  invariantGraph.addInvariantNode(
      std::make_unique<Int2BoolNode>(invariantGraph.retrieveVarNode(intArg),
                                     invariantGraph.retrieveVarNode(boolArg)));
  return true;
}

bool bool2int(FznInvariantGraph& invariantGraph,
              const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool2int") {
    return false;
  }

  verifyNumArguments(constraint, 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::BoolArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)

  if (constraint.definedVar().has_value() &&
      std::holds_alternative<std::shared_ptr<fznparser::BoolVar>>(
          constraint.definedVar().value())) {
    return int2bool(invariantGraph,
                    std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                    std::get<fznparser::IntArg>(constraint.arguments().at(1)));
  }

  return bool2int(invariantGraph,
                  std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                  std::get<fznparser::IntArg>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn
