#pragma once

#include <fznparser/model.hpp>
#include <map>

#include "core/propagationEngine.hpp"

namespace search {

class VariableStore {
 public:
  using VariableMap =
      std::map<VarId, std::shared_ptr<fznparser::SearchVariable>>;

  struct Domain {
    Int lowerBound;
    Int upperBound;

    Domain(Int lb, Int ub) : lowerBound(lb), upperBound(ub) {}
  };

  VariableStore(const PropagationEngine& engine, const VariableMap& variable);

  Domain domain(VarId variable);

 private:
  const PropagationEngine& _engine;
  std::vector<std::optional<Domain>> _domains;
};

}  // namespace search
