#pragma once

#include <unordered_set>
#include <vector>

#include "atlantis/propagation/propagation/outputToInputExplorer.hpp"
#include "atlantis/propagation/propagation/propagationGraph.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

class Solver : public SolverBase {
 protected:
  PropagationMode _propagationMode;
  size_t _numVars{0};

  PropagationGraph _propGraph;
  OutputToInputExplorer _outputToInputExplorer;

  std::vector<bool> _isEnqueued;
  std::vector<std::vector<VarId>> _layerQueue{};
  std::vector<size_t> _layerQueueIndex{};

  std::unordered_set<VarId> _modifiedSearchVars;

  void incCurrentTimestamp();

  void closeInvariants();

  void clearPropagationQueue();

  void propagateOnClose();

  template <CommitMode Mode, bool SingleLayer>
  void propagate();

  void outputToInputPropagate();

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param definedVarId the variable that is defined by the invariant
   * @param invariantId the invariant defining the variable
   * @throw VarAlreadyDefinedException if the variable is already defined by an
   * invariant.
   */
  void registerDefinedVar(VarId definedVarId, InvariantId invariantId) final;

 public:
  Solver(/* args */);

  void open() final;
  void close() final;
  void computeBounds() final;

  void setPropagationMode(PropagationMode);
  OutputToInputMarkingMode outputToInputMarkingMode() const;
  void setOutputToInputMarkingMode(OutputToInputMarkingMode);

  //--------------------- Notification ---------------------
  void enqueueDefinedVar(VarId) final;
  void enqueueDefinedVar(VarId, size_t layer);

  [[nodiscard]] PropagationMode propagationMode() const {
    return _propagationMode;
  }

  // --------------------- Activity ----------------
  [[nodiscard]] VarId dequeueComputedVar(Timestamp);

  //--------------------- Propagation ---------------------
  void beginMove();
  void endMove();
  void setValue(Timestamp, VarId, Int val);
  void setValue(Timestamp, VarViewId, Int val);

  void setValue(VarId id, Int val) { setValue(_currentTimestamp, id, val); }

  void setValue(VarViewId id, Int val) { setValue(_currentTimestamp, id, val); }

  void beginProbe();
  void endProbe();
  void query(VarViewId);

  void beginCommit();
  void endCommit();

  size_t numVars() const;
  size_t numInvariants() const;

  [[nodiscard]] const std::vector<VarId>& searchVars() const;
  [[nodiscard]] const std::unordered_set<VarId>& modifiedSearchVar() const;
  [[nodiscard]] const std::vector<std::pair<VarId, bool>>& inputVars(
      InvariantId) const;

  /**
   * returns the next input at the current timestamp.
   */
  VarId nextInput(InvariantId);

  InvariantId definingInvariant(VarViewId) const;

  [[nodiscard]] const std::vector<VarId>& varsDefinedBy(InvariantId) const;

  [[nodiscard]] const std::vector<PropagationGraph::ListeningInvariantData>&
      listeningInvariantData(VarId) const;

  /**
   * Notify an invariant that its current input has changed
   */
  void notifyCurrentInputChanged(InvariantId);

  /**
   * Register that a variable is a input to an invariant
   * @param invariantId the invariant
   * @param inputId the id of the variable
   * @param localId the id of the variable in the invariant
   * @param isDynamicInput true if the input is a dynamic input to the invariant
   */
  void registerInvariantInput(InvariantId invariantId, VarViewId inputId,
                              LocalId localId, bool isDynamicInput) final;

  void registerVar(VarId) final;
  void registerInvariant(InvariantId) final;
};

}  // namespace atlantis::propagation
