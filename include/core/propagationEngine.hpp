#pragma once

#include <queue>

#include "core/engine.hpp"
#include "propagation/outputToInputExplorer.hpp"
#include "propagation/propagationGraph.hpp"
#include "utils/idMap.hpp"

class PropagationEngine : public Engine {
 public:
  enum class PropagationMode { INPUT_TO_OUTPUT, OUTPUT_TO_INPUT, MIXED };
  const bool _useMarkingForOutputToInput = true;

 protected:
  PropagationMode _mode;

 public:
  const PropagationMode& mode = _mode;

 protected:
  size_t _numVariables;

  PropagationGraph _propGraph;
  OutputToInputExplorer _outputToInputExplorer;

  IdMap<VarIdBase, bool> _isEnqueued;

  IdMap<VarIdBase, bool> _varIsOnPropagationPath;
  std::queue<VarIdBase> _propagationPathQueue;

  void recomputeAndCommit();

  void emptyModifiedVariables();

  template <bool DoCommit>
  void propagate();

  template <bool OutputToInputMarking>
  void outputToInputPropagate();

  void markPropagationPathAndEmptyModifiedVariables();
  void clearPropagationPath();

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param definedVarId the variable that is defined by the invariant
   * @param invariantId the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVariable(VarId definedVarId,
                               InvariantId invariantId) override;

 public:
  PropagationEngine(/* args */);

  void open() override;
  void close() override;

  void setPropagationMode(PropagationMode);

  //--------------------- Notificaion ---------------------
  /***
   * @param ts the timestamp when the changed happened
   * @param id the id of the changed variable
   */
  void notifyMaybeChanged(Timestamp ts, VarId id) override;
  void queueForPropagation(Timestamp, VarId) override;

  // todo: Maybe there is a better word than "active", like "relevant".
  // --------------------- Activity ----------------
  /**
   * returns true if variable id is relevant for propagation.
   * Note that this is not the same thing as the variable being modified.
   */
  bool isOnPropagationPath(VarId);
  /**
   * returns true if invariant id is relevant for propagation.
   * Note that this is not the same thing as the invariant being modified.
   */
  static bool isOnPropagationPath(Timestamp, InvariantId) { return true; }

  [[nodiscard]] VarId getNextStableVariable(Timestamp);

  //--------------------- Move semantics ---------------------
  void beginMove();
  void endMove();
  void setValue(Timestamp, VarId, Int val);
  inline void setValue(VarId id, Int val) { setValue(_currentTime, id, val); }

  void beginQuery();
  void endQuery();
  void query(VarId);

  void beginCommit();
  void endCommit();

  /**
   * returns the next parameter at the current timestamp.
   */
  VarId getNextParameter(InvariantId);

  InvariantId getDefiningInvariant(VarId);

  // This function is used by propagation, which is unaware of views.
  [[nodiscard]] inline bool hasChanged(Timestamp, VarId) const;

  [[nodiscard]] const std::vector<VarIdBase>& getVariablesDefinedBy(
      InvariantId) const;

  /**
   * Notify an invariant that its current parameter has changed
   */
  void notifyCurrentParameterChanged(InvariantId);

  /**
   * Register that a variable is a parameter to an invariant
   * @param invariantId the invariant
   * @param parameterId the id of the variable
   * @param localId the id of the variable in the invariant
   */
  void registerInvariantParameter(InvariantId invariantId, VarId parameterId,
                                  LocalId localId) override;

  void registerVar(VarId) override;
  void registerInvariant(InvariantId) override;

  PropagationGraph& getPropGraph();
};

inline InvariantId PropagationEngine::getDefiningInvariant(VarId id) {
  // Returns NULL_ID is not defined.
  return _propGraph.getDefiningInvariant(id);
}

inline void PropagationEngine::clearPropagationPath() {
  _varIsOnPropagationPath.assign_all(false);
}

inline bool PropagationEngine::isOnPropagationPath(VarId id) {
  return !_useMarkingForOutputToInput || _varIsOnPropagationPath.get(id);
}

inline const std::vector<VarIdBase>& PropagationEngine::getVariablesDefinedBy(
    InvariantId id) const {
  return _propGraph.getVariablesDefinedBy(id);
}

inline VarId PropagationEngine::getNextParameter(InvariantId id) {
  return getSourceId(
      _store.getInvariant(id).getNextParameter(_currentTime, *this));
}
inline void PropagationEngine::notifyCurrentParameterChanged(InvariantId id) {
  _store.getInvariant(id).notifyCurrentParameterChanged(_currentTime, *this);
}

inline bool PropagationEngine::hasChanged(Timestamp ts, VarId id) const {
  assert(id.idType != VarIdType::view);
  return _store.getConstIntVar(id).hasChanged(ts);
}

inline void PropagationEngine::setValue(Timestamp ts, VarId id, Int val) {
  assert(id.idType != VarIdType::view);
  _store.getIntVar(id).setValue(ts, val);
  notifyMaybeChanged(ts, id);
}

inline void PropagationEngine::setPropagationMode(PropagationMode m) {
  assert(_isOpen);
  _mode = m;
}

template <bool OutputToInputMarking>
inline void PropagationEngine::outputToInputPropagate() {
  _outputToInputExplorer.propagate<OutputToInputMarking>(_currentTime);
}
