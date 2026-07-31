#pragma once

#include <memory>
#include <vector>

#include "atlantis/misc/blackboxFunction.hpp"
#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for outputs <- blackBoxFn(inputs)
 *
 * The blackbox function is shared with the equivalent invariant in every other
 * search thread; it hands each thread its own private instance internally.
 */
class Blackbox : public Invariant {
 private:
  std::shared_ptr<blackbox::BlackBoxFn> _blackBoxFn;
  std::vector<VarId> _outputs;
  std::vector<VarViewId> _inputs;

  // Buffers reused across invocations: recompute runs on every input change,
  // so allocating them per call would dominate the cost of cheap blackboxes.
  std::vector<Int> _intIn;
  std::vector<double> _floatIn;
  std::vector<Int> _intOut;
  std::vector<double> _floatOut;

 public:
  Blackbox(SolverBase&, std::shared_ptr<blackbox::BlackBoxFn> blackBoxFn,
           std::vector<VarId>&& outputs, std::vector<VarViewId>&& inputs);

  void registerVars() override;
  /// The range of a blackbox cannot be inferred, so the outputs keep the bounds
  /// declared for them by the model.
  void updateBounds(bool) override {}
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
