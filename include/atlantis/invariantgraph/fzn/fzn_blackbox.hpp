#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>
#include <memory>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/misc/blackboxFunction.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_blackbox(FznInvariantGraph&,
                  std::shared_ptr<blackbox::BlackBoxFn> blackboxFn,
                  const std::shared_ptr<fznparser::IntVarArray>& int_in,
                  const std::shared_ptr<fznparser::FloatVarArray>& float_in,
                  const std::shared_ptr<fznparser::IntVarArray>& int_out,
                  const std::shared_ptr<fznparser::FloatVarArray>& float_out);

bool fzn_blackbox(FznInvariantGraph&, const fznparser::Constraint& constraint);

}  // namespace atlantis::invariantgraph::fzn
