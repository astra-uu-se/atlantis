#pragma once

#include <utility>
#include <variant>

#include "core/domains.hpp"
#include "core/types.hpp"
#include "randomProvider.hpp"

namespace search {

class SearchVariable {
 private:
  SearchDomain _domain;
  VarId _varId;

 public:
  SearchVariable(VarId varId, SearchDomain domain)
      : _domain(std::move(domain)), _varId(varId) {}

  [[nodiscard]] VarId engineId() const noexcept { return _varId; }

  [[nodiscard]] SearchDomain& domain() noexcept { return _domain; }
};

}  // namespace search