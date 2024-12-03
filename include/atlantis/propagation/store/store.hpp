#pragma once

#include <memory>
#include <vector>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/variables/intVar.hpp"
#include "atlantis/propagation/views/intView.hpp"

namespace atlantis::propagation {

class Store {
 private:
  std::vector<IntVar> _intVars;
  std::vector<std::unique_ptr<Invariant>> _invariants;
  std::vector<std::unique_ptr<IntView>> _intViews;
  std::vector<VarId> _intViewSourceId;

 public:
  Store();

  VarViewId createIntVar(Timestamp ts, Int initValue, Int lowerBound,
                         Int upperBound);

  InvariantId createInvariantFromPtr(std::unique_ptr<Invariant>&&);

  VarViewId createIntViewFromPtr(std::unique_ptr<IntView>&&);

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

  [[nodiscard]] std::vector<std::unique_ptr<Invariant>>::iterator
  invariantBegin();

  [[nodiscard]] std::vector<std::unique_ptr<Invariant>>::iterator
  invariantEnd();

  [[nodiscard]] size_t numVars() const;

  [[nodiscard]] size_t numInvariants() const;

  [[nodiscard]] VarId dynamicInputVar(Timestamp, InvariantId) const noexcept;
};

}  // namespace atlantis::propagation
