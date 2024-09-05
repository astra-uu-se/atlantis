#pragma once

#include "atlantis/propagation/types.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/types.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::search {

class SearchVar {
 private:
  SearchDomain _domain;
  propagation::VarViewId _varId{propagation::NULL_ID};

 public:
  explicit SearchVar(propagation::VarViewId varId, SearchDomain&& domain)
      : _domain(std::move(domain)), _varId(varId) {}

  [[nodiscard]] propagation::VarViewId solverId() const noexcept {
    return _varId;
  }

  [[nodiscard]] SearchDomain& domain() noexcept { return _domain; }
  [[nodiscard]] const SearchDomain& constDomain() const noexcept {
    return _domain;
  }
  [[nodiscard]] bool isFixed() const noexcept { return _domain.isFixed(); }
};

}  // namespace atlantis::search
