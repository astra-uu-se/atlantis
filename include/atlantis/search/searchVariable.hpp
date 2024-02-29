#pragma once

#include <utility>
#include <variant>

#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"
#include "atlantis/utils/domains.hpp"
#include "randomProvider.hpp"

namespace atlantis::search {

class SearchVar {
 private:
  SearchDomain _domain;
  propagation::VarId _varId;

 public:
  explicit SearchVar(propagation::VarId varId, SearchDomain&& domain)
      : _domain(std::move(domain)), _varId(varId) {}

  [[nodiscard]] propagation::VarId solverId() const noexcept { return _varId; }

  [[nodiscard]] SearchDomain& domain() noexcept { return _domain; }
  [[nodiscard]] bool isFixed() const noexcept { return _domain.isFixed(); }
};

}  // namespace atlantis::search
