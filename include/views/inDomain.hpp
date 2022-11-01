#pragma once

#include <cassert>
#include <limits>

#include "core/engine.hpp"
#include "core/types.hpp"
#include "variables/committable.hpp"
#include "variables/intVar.hpp"
#include "views/intView.hpp"

class Engine;
class InDomain : public IntView {
 private:
  const std::vector<DomainEntry> _domain;
  const VarId _x;
  Committable<std::pair<Int, Int>> _cache;

  Int compute(const Int val) const;

 public:
  explicit InDomain(Engine& engine, VarId parentId,
                    std::vector<DomainEntry>&& domain);

  [[nodiscard]] Int value(Timestamp) override;
  [[nodiscard]] Int committedValue() override;
  [[nodiscard]] Int lowerBound() const override;
  [[nodiscard]] Int upperBound() const override;
};
