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
   * returns the next dependency at the current timestamp.
   */
  VarId getNextDependency(InvariantId);

  InvariantId getDefiningInvariant(VarId);

  // This function is used by propagation, which is unaware of views.
  [[nodiscard]] inline bool hasChanged(Timestamp, VarId) const;

  [[nodiscard]] const std::vector<VarIdBase>& getVariablesDefinedBy(
      InvariantId) const;

  /**
   * Notify an invariant that its current dependency has changed
   */
  void notifyCurrentDependencyChanged(InvariantId);

  /**
   * Register that Invariant to depends on variable from depends on dependency
   * @param dependentInvariant the invariant that the variable depends on
   * @param sourceVar the depending variable
   * @param localId the id of the depending variable in the invariant
   */
  void registerInvariantDependsOnVar(InvariantId dependentInvariant,
                                     VarId sourceVar, LocalId localId) override;

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

inline VarId PropagationEngine::getNextDependency(InvariantId id) {
  return getSourceId(
      _store.getInvariant(id).getNextDependency(_currentTime, *this));
}
inline void PropagationEngine::notifyCurrentDependencyChanged(InvariantId id) {
  _store.getInvariant(id).notifyCurrentDependencyChanged(_currentTime, *this);
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
