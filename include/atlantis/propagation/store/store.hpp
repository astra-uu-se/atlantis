#pragma once

#include <memory>
#include <vector>

#include "atlantis/propagation/types.hpp"

namespace atlantis::propagation {

class Invariant;
class IntVar;
class IntView;

class Store {
  std::vector<IntVar> _intVars;
  std::vector<std::shared_ptr<Invariant>> _invariants;
  std::vector<std::shared_ptr<IntView>> _intViews;
  std::vector<VarId> _intViewSourceId;

 public:
  Store();

  VarViewId createIntVar(Timestamp ts, Int initValue, Int lowerBound,
                         Int upperBound);

  InvariantId createInvariantFromPtr(const std::shared_ptr<Invariant>&);

  VarViewId createIntViewFromPtr(const std::shared_ptr<IntView>&);

  [[nodiscard]] IntVar& intVar(VarId);

  [[nodiscard]] const IntVar& constIntVar(VarId) const;

  [[nodiscard]] IntView& intView(ViewId);

  [[nodiscard]] const IntView& constIntView(ViewId) const;

  [[nodiscard]] VarId sourceId(VarViewId) const noexcept;

  [[nodiscard]] VarId intViewSourceId(ViewId) const;

  [[nodiscard]] Invariant& invariant(InvariantId);

  [[nodiscard]] const Invariant& constInvariant(InvariantId) const;

  [[nodiscard]] std::vector<IntVar>::iterator intVarBegin();

  [[nodiscard]] std::vector<IntVar>::iterator intVarEnd();

  [[nodiscard]] std::vector<std::shared_ptr<Invariant>>::iterator
  invariantBegin();

  [[nodiscard]] std::vector<std::shared_ptr<Invariant>>::iterator
  invariantEnd();

  [[nodiscard]] size_t numVars() const;

  [[nodiscard]] size_t numInvariants() const;

  [[nodiscard]] VarId dynamicInputVar(Timestamp, InvariantId) const noexcept;

  [[nodiscard]] std::vector<Int> currentValues() const;
};

}  // namespace atlantis::propagation
