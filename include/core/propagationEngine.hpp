#pragma once

#include <queue>

#include "core/engine.hpp"
#include "exceptions/exceptions.hpp"
#include "propagation/outputToInputExplorer.hpp"
#include "propagation/propagationGraph.hpp"
#include "utils/hashes.hpp"
#include "utils/idMap.hpp"

class PropagationEngine : public Engine {
 protected:
  PropagationMode _propagationMode;

 public:
  const PropagationMode& propagationMode = _propagationMode;

 protected:
  size_t _numVariables;

  PropagationGraph _propGraph;
  OutputToInputExplorer _outputToInputExplorer;

  IdMap<VarIdBase, bool> _isEnqueued;

  std::unordered_set<VarIdBase> _modifiedDecisionVariables;
  Timestamp _decisionVariablesModifiedAt;

  void recomputeAndCommit();

  void clearPropagationQueue();

  template <bool DoCommit>
  void propagate();

  void outputToInputPropagate();

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param definedVarId the variable that is defined by the invariant
   * @param invariantId the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVariable(VarId definedVarId,
                               InvariantId invariantId) final;

 public:
  PropagationEngine(/* args */);

  void open() final;
  void close() final;

  void setPropagationMode(PropagationMode);
  OutputToInputMarkingMode outputToInputMarkingMode() const;
  void setOutputToInputMarkingMode(OutputToInputMarkingMode);

  //--------------------- Notificaion ---------------------
  /***
   * @param ts the timestamp when the changed happened
   * @param id the id of the changed variable
   */
  void notifyMaybeChanged(Timestamp ts, VarId id) final;
  void queueForPropagation(Timestamp, VarId) final;

  // todo: Maybe there is a better word than "active", like "relevant".
  // --------------------- Activity ----------------
  [[nodiscard]] VarId getNextStableVariable(Timestamp);

  //--------------------- Move semantics ---------------------
  void beginMove();
  void endMove();
  void setValue(Timestamp, VarId, Int val);
  inline void setValue(VarId id, Int val) {
    setValue(_currentTimestamp, id, val);
  }

  void beginQuery();
  void endQuery();
  void query(VarId);

  void beginCommit();
  void endCommit();

  size_t getNumVariables();
  size_t getNumInvariants();

  [[nodiscard]] const std::vector<VarIdBase>& getDecisionVariables() const;
  [[nodiscard]] const std::unordered_set<VarIdBase>&
  getModifiedDecisionVariables() const;
  [[nodiscard]] const std::vector<VarIdBase>& getOutputVariables() const;
  [[nodiscard]] const std::vector<VarIdBase>& getInputVariables(
      InvariantId) const;

  /**
   * returns the next input at the current timestamp.
   */
  VarId getNextInput(InvariantId);

  InvariantId getDefiningInvariant(VarId);

  // This function is used by propagation, which is unaware of views.
  [[nodiscard]] inline bool hasChanged(Timestamp, VarId) const;

  [[nodiscard]] const std::vector<VarIdBase>& getVariablesDefinedBy(
      InvariantId) const;

  [[nodiscard]] const std::vector<InvariantId>& getListeningInvariants(
      VarId) const;

  /**
   * Notify an invariant that its current input has changed
   */
  void notifyCurrentInputChanged(InvariantId);

  /**
   * Register that a variable is a input to an invariant
   * @param invariantId the invariant
   * @param inputId the id of the variable
   * @param localId the id of the variable in the invariant
   */
  void registerInvariantInput(InvariantId invariantId, VarId inputId,
                              LocalId localId) final;

  void registerVar(VarId) final;
  void registerInvariant(InvariantId) final;

  PropagationGraph& getPropGraph();
};

inline size_t PropagationEngine::getNumVariables() {
  return _propGraph.getNumVariables();
}

inline size_t PropagationEngine::getNumInvariants() {
  return _propGraph.getNumInvariants();
}

inline InvariantId PropagationEngine::getDefiningInvariant(VarId id) {
  // Returns NULL_ID is not defined.
  return _propGraph.getDefiningInvariant(id);
}

inline const std::vector<VarIdBase>& PropagationEngine::getVariablesDefinedBy(
    InvariantId invariantId) const {
  return _propGraph.getVariablesDefinedBy(invariantId);
}

inline const std::vector<InvariantId>&
PropagationEngine::getListeningInvariants(VarId id) const {
  return _propGraph.getListeningInvariants(id);
}

inline VarId PropagationEngine::getNextInput(InvariantId invariantId) {
  return getSourceId(
      _store.getInvariant(invariantId).getNextInput(_currentTimestamp, *this));
}
inline void PropagationEngine::notifyCurrentInputChanged(
    InvariantId invariantId) {
  _store.getInvariant(invariantId)
      .notifyCurrentInputChanged(_currentTimestamp, *this);
}

inline bool PropagationEngine::hasChanged(Timestamp ts, VarId id) const {
  assert(id.idType != VarIdType::view);
  return _store.getConstIntVar(id).hasChanged(ts);
}

inline void PropagationEngine::setValue(Timestamp ts, VarId id, Int val) {
  assert(id.idType != VarIdType::view);
  assert(_propGraph.isDecisionVar(id));

  IntVar& var = _store.getIntVar(id);
  var.setValue(ts, val);

  if (_propagationMode == PropagationMode::OUTPUT_TO_INPUT) {
    if (ts != _decisionVariablesModifiedAt) {
      _decisionVariablesModifiedAt = ts;
      _modifiedDecisionVariables.clear();
    }

    if (var.hasChanged(ts)) {
      _modifiedDecisionVariables.emplace(id);
    } else {
      _modifiedDecisionVariables.erase(id);
    }
  }

  notifyMaybeChanged(ts, id);
}

inline void PropagationEngine::setPropagationMode(PropagationMode propMode) {
  if (!_isOpen) {
    throw EngineClosedException(
        "Cannot set propagation mode when model is closed");
  }
  _propagationMode = propMode;
}

inline OutputToInputMarkingMode PropagationEngine::outputToInputMarkingMode()
    const {
  return _outputToInputExplorer.outputToInputMarkingMode();
}

inline void PropagationEngine::setOutputToInputMarkingMode(
    OutputToInputMarkingMode markingMode) {
  if (!_isOpen) {
    throw EngineClosedException(
        "Cannot set output-to-input marking mode when model is closed");
  }
  _outputToInputExplorer.setOutputToInputMarkingMode(markingMode);
}

inline const std::vector<VarIdBase>& PropagationEngine::getDecisionVariables() {
  return _propGraph._decisionVariables;
}

inline const std::vector<VarIdBase>& PropagationEngine::getOutputVariables()
    const {
  return _propGraph._outputVariables;
}

inline const std::vector<VarIdBase>& PropagationEngine::getInputVariables(
    InvariantId invariantId) const {
  return _propGraph.getInputVariables(invariantId);
}

inline const std::unordered_set<VarIdBase>&
PropagationEngine::getModifiedDecisionVariables() const {
  assert(_currentTimestamp == _decisionVariablesModifiedAt);
  return _modifiedDecisionVariables;
}

inline void PropagationEngine::outputToInputPropagate() {
  assert(propagationMode == PropagationMode::OUTPUT_TO_INPUT);
  _outputToInputExplorer.propagate(_currentTimestamp);
}
