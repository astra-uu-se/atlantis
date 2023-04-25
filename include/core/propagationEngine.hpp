#pragma once

#include <deque>
#include <iostream>
#include <queue>
#include <set>

#include "core/engine.hpp"
#include "exceptions/exceptions.hpp"
#include "propagation/outputToInputExplorer.hpp"
#include "propagation/propagationGraph.hpp"
#include "utils/hashes.hpp"
#include "utils/idMap.hpp"

class PropagationEngine : public Engine {
 protected:
  PropagationMode _propagationMode;

 protected:
  size_t _numVariables{0};

  PropagationGraph _propGraph;
  OutputToInputExplorer _outputToInputExplorer;

  IdMap<VarIdBase, bool> _isEnqueued;
  std::vector<std::vector<VarIdBase>> _layerQueue{};
  std::vector<size_t> _layerQueueIndex{};

  std::unordered_set<VarIdBase> _modifiedSearchVariables;

  void incCurrentTimestamp();

  void closeInvariants();
  void recomputeAndCommit();

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
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVariable(VarId definedVarId,
                               InvariantId invariantId) final;

 public:
  PropagationEngine(/* args */);

  void open() final;
  void close() final;
  void computeBounds() final;

  void setPropagationMode(PropagationMode);
  OutputToInputMarkingMode outputToInputMarkingMode() const;
  void setOutputToInputMarkingMode(OutputToInputMarkingMode);

  //--------------------- Notificaion ---------------------
  /***
   * @param id the id of the changed variable
   */
  void enqueueComputedVar(VarId) final;
  void enqueueComputedVar(VarId, size_t layer);

  [[nodiscard]] inline PropagationMode propagationMode() const {
    return _propagationMode;
  }

  // --------------------- Activity ----------------
  [[nodiscard]] VarId dequeueComputedVar(Timestamp);

  //--------------------- Propagation ---------------------
  void beginMove();
  void endMove();
  void setValue(Timestamp, VarId, Int val);
  inline void setValue(VarId id, Int val) {
    setValue(_currentTimestamp, id, val);
  }

  void beginProbe();
  void endProbe();
  void query(VarId);

  void beginCommit();
  void endCommit();

  size_t numVariables();
  size_t numInvariants();

  [[nodiscard]] const std::vector<VarIdBase>& searchVariables() const;
  [[nodiscard]] const std::unordered_set<VarIdBase>& modifiedSearchVariables()
      const;
  [[nodiscard]] const std::vector<VarIdBase>& evaluationVariables() const;
  [[nodiscard]] const std::vector<std::pair<VarIdBase, bool>>& inputVariables(
      InvariantId) const;

  /**
   * returns the next input at the current timestamp.
   */
  VarId nextInput(InvariantId);

  InvariantId definingInvariant(VarId);

  // This function is used by propagation, which is unaware of views.
  [[nodiscard]] inline bool hasChanged(Timestamp, VarId) const;

  [[nodiscard]] const std::vector<VarIdBase>& variablesDefinedBy(
      InvariantId) const;

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
   */
  void registerInvariantInput(InvariantId invariantId, VarId inputId,
                              LocalId localId, bool isDynamic = false) final;

  void registerVar(VarId) final;
  void registerInvariant(InvariantId) final;

  PropagationGraph& propGraph();
};

inline void PropagationEngine::incCurrentTimestamp() {
  ++_currentTimestamp;
  if (_propagationMode == PropagationMode::INPUT_TO_OUTPUT) {
    clearPropagationQueue();
  } else {
    _modifiedSearchVariables.clear();
  }
  assert(std::all_of(searchVariables().begin(), searchVariables().end(),
                     [&](const VarIdBase varId) {
                       return !_store.intVar(varId).hasChanged(
                           _currentTimestamp);
                     }));
}

inline size_t PropagationEngine::numVariables() {
  return _propGraph.numVariables();
}

inline size_t PropagationEngine::numInvariants() {
  return _propGraph.numInvariants();
}

inline InvariantId PropagationEngine::definingInvariant(VarId id) {
  // Returns NULL_ID if there is no defining invariant
  return _propGraph.definingInvariant(id);
}

inline const std::vector<VarIdBase>& PropagationEngine::variablesDefinedBy(
    InvariantId invariantId) const {
  return _propGraph.variablesDefinedBy(invariantId);
}

inline const std::vector<PropagationGraph::ListeningInvariantData>&
PropagationEngine::listeningInvariantData(VarId id) const {
  return _propGraph.listeningInvariantData(id);
}

inline VarId PropagationEngine::nextInput(InvariantId invariantId) {
  return sourceId(_store.invariant(invariantId).nextInput(_currentTimestamp));
}
inline void PropagationEngine::notifyCurrentInputChanged(
    InvariantId invariantId) {
  _store.invariant(invariantId).notifyCurrentInputChanged(_currentTimestamp);
}

inline bool PropagationEngine::hasChanged(Timestamp ts, VarId id) const {
  assert(id.idType != VarIdType::view);
  return _store.constIntVar(id).hasChanged(ts);
}

inline void PropagationEngine::setValue(Timestamp ts, VarId id, Int val) {
  assert(id.idType != VarIdType::view);
  assert(_propGraph.isSearchVariable(id));

  IntVar& var = _store.intVar(id);
  var.setValue(ts, val);

  if (_propagationMode == PropagationMode::OUTPUT_TO_INPUT) {
    if (ts != _currentTimestamp) {
      _modifiedSearchVariables.clear();
    }

    if (var.hasChanged(ts)) {
      _modifiedSearchVariables.emplace(id);
    } else {
      _modifiedSearchVariables.erase(id);
    }
  }
  enqueueComputedVar(id);
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

inline const std::vector<VarIdBase>& PropagationEngine::searchVariables()
    const {
  return _propGraph.searchVariables();
}

inline const std::vector<VarIdBase>& PropagationEngine::evaluationVariables()
    const {
  return _propGraph.evaluationVariables();
}

inline const std::vector<std::pair<VarIdBase, bool>>&
PropagationEngine::inputVariables(InvariantId invariantId) const {
  return _propGraph.inputVariables(invariantId);
}

inline const std::unordered_set<VarIdBase>&
PropagationEngine::modifiedSearchVariables() const {
  return _modifiedSearchVariables;
}

inline void PropagationEngine::outputToInputPropagate() {
  assert(propagationMode() == PropagationMode::OUTPUT_TO_INPUT);
  _outputToInputExplorer.propagate(_currentTimestamp);
}
