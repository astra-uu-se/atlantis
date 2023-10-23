#pragma once

#include <utility>
#include <variant>

#include "propagation/types.hpp"
#include "randomProvider.hpp"
#include "types.hpp"
#include "utils/domains.hpp"

namespace atlantis::search {

class SearchVariable {
 private:
  SearchDomain _domain;
  propagation::VarId _varId;

 public:
  explicit SearchVariable(propagation::VarId varId, const SearchDomain& domain)
      : _domain(domain), _varId(varId) {}

  [[nodiscard]] propagation::VarId solverId() const noexcept { return _varId; }

  [[nodiscard]] SearchDomain& domain() noexcept { return _domain; }
  [[nodiscard]] bool isFixed() const noexcept { return _domain.isFixed(); }
};

}  // namespace atlantis::search