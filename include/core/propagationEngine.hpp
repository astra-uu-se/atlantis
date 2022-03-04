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

 protected:
  size_t _numVariables;

  PropagationGraph _propGraph;
  OutputToInputExplorer _outputToInputExplorer;

  IdMap<VarIdBase, bool> _isEnqueued;

  std::unordered_set<VarIdBase> _modifiedSearchVariables;
  Timestamp _searchVariablesModifiedAt;

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

  [[nodiscard]] inline PropagationMode propagationMode() const {
    return _propagationMode;
  }

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
  [[nodiscard]] const std::vector<VarIdBase>& inputVariables(InvariantId) const;

  /**
   * returns the next input at the current timestamp.
   */
  VarId nextInput(InvariantId);

  InvariantId definingInvariant(VarId);

  // This function is used by propagation, which is unaware of views.
  [[nodiscard]] inline bool hasChanged(Timestamp, VarId) const;

  [[nodiscard]] const std::vector<VarIdBase>& variablesDefinedBy(
      InvariantId) const;

  [[nodiscard]] const std::vector<InvariantId>& listeningInvariants(
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

  PropagationGraph& propGraph();
};

inline size_t PropagationEngine::numVariables() {
  return _propGraph.numVariables();
}

inline size_t PropagationEngine::numInvariants() {
  return _propGraph.numInvariants();
}

inline InvariantId PropagationEngine::definingInvariant(VarId id) {
  // Returns NULL_ID is not defined.
  return _propGraph.definingInvariant(id);
}

inline const std::vector<VarIdBase>& PropagationEngine::getVariablesDefinedBy(
    InvariantId invariantId) const {
  return _propGraph.variablesDefinedBy(invariantId);
}

inline const std::vector<InvariantId>& PropagationEngine::listeningInvariants(
    VarId id) const {
  return _propGraph.listeningInvariants(id);
}

inline VarId PropagationEngine::nextInput(InvariantId invariantId) {
  return sourceId(
      _store.invariant(invariantId).nextInput(_currentTimestamp, *this));
}
inline void PropagationEngine::notifyCurrentInputChanged(
    InvariantId invariantId) {
  _store.invariant(invariantId)
      .notifyCurrentInputChanged(_currentTimestamp, *this);
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
    if (ts != _searchVariablesModifiedAt) {
      _searchVariablesModifiedAt = ts;
      _modifiedSearchVariables.clear();
    }

    if (var.hasChanged(ts)) {
      _modifiedSearchVariables.emplace(id);
    } else {
      _modifiedSearchVariables.erase(id);
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

inline const std::vector<VarIdBase>& PropagationEngine::getSearchVariables()
    const {
  return _propGraph._searchVariables;
}

inline const std::vector<VarIdBase>& PropagationEngine::evaluationVariables()
    const {
  return _propGraph._evaluationVariables;
}

inline const std::vector<VarIdBase>& PropagationEngine::inputVariables(
    InvariantId invariantId) const {
  return _propGraph.inputVariables(invariantId);
}

inline const std::unordered_set<VarIdBase>&
PropagationEngine::modifiedSearchVariables() const {
  assert(_currentTimestamp == _searchVariablesModifiedAt);
  return _modifiedSearchVariables;
}

inline void PropagationEngine::outputToInputPropagate() {
  assert(propagationMode == PropagationMode::OUTPUT_TO_INPUT);
  _outputToInputExplorer.propagate(_currentTimestamp);
}
