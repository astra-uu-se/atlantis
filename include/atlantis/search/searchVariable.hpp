#pragma once

#include <memory>

#include "atlantis/propagation/types.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis {
class SearchDomain;
}

namespace atlantis::search {

class SearchVar {
  std::shared_ptr<const SearchDomain> _domain;
  propagation::VarId _varId{propagation::NULL_ID};

 public:
  explicit SearchVar(propagation::VarViewId varId,
                     const std::shared_ptr<const SearchDomain>& domain)
      : _domain(domain), _varId(propagation::VarId{varId}) {
    assert(varId.isVar());
  }

  explicit SearchVar(propagation::VarId varId,
                     const std::shared_ptr<const SearchDomain>& domain)
      : _domain(domain), _varId(varId) {}

  [[nodiscard]] propagation::VarId solverId() const noexcept { return _varId; }

  [[nodiscard]] std::shared_ptr<const SearchDomain> domain() const noexcept {
    return _domain;
  }
  [[nodiscard]] bool isFixed() const noexcept { return _domain->isFixed(); }
};

}  // namespace atlantis::search
