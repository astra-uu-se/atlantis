#pragma once

#include <queue>

#include "core/engine.hpp"
#include "misc/logging.hpp"
#include "propagation/outputToInputExplorer.hpp"
#include "propagation/propagationGraph.hpp"
#include "utils/idMap.hpp"

class PropagationEngine : public Engine {
 public:
  enum class PropagationMode { INPUT_TO_OUTPUT, OUTPUT_TO_INPUT, MIXED };
  const bool m_useMarkingForOutputToInput = true;

 protected:
  PropagationMode m_mode;
  size_t m_numVariables;

  PropagationGraph m_propGraph;
  OutputToInputExplorer m_outputToInputExplorer;

  IdMap<VarId, bool> m_isEnqueued;

  IdMap<VarId, bool> m_varIsOnPropagationPath;
  std::queue<VarId> m_propagationPathQueue;

  void recomputeAndCommit();

  void emptyModifiedVariables();

  template <bool DoCommit>
  void propagate();

  template <bool DoCommit>
  void outputToInputePropagate();

  void markPropagationPathAndEmptyModifiedVariables();
  void clearPropagationPath();

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param dependent the variable that is defined by the invariant
   * @param source the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  void registerDefinedVariable(VarId dependent, InvariantId source) override;

 public:
  PropagationEngine(/* args */);

  void open() override;
  void close() override;

  void setPropagationMode(PropagationMode m);
  PropagationMode getPropagationMode();

  //--------------------- Notificaion ---------------------
  /***
   * @param t the timestamp when the changed happened
   * @param id the id of the changed variable
   */
  void notifyMaybeChanged(Timestamp t, VarId id) override;
  void queueForPropagation(Timestamp t, VarId id) override;

  // todo: Maybe there is a better word than "active", like "relevant".
  // --------------------- Activity ----------------
  /**
   * returns true if variable id is relevant for propagation.
   * Note that this is not the same thing as the variable being modified.
   */
  bool isOnPropagationPath(VarId v);
  /**
   * returns true if invariant id is relevant for propagation.
   * Note that this is not the same thing as the invariant being modified.
   */
  static bool isOnPropagationPath(Timestamp, InvariantId) { return true; }

  [[nodiscard]] VarId getNextStableVariable(Timestamp t);

  //--------------------- Move semantics ---------------------
  void beginMove();
  void endMove();
  void setValue(Timestamp, VarId, Int val);
  inline void setValue(VarId v, Int val) { setValue(m_currentTime, v, val); }

  void beginQuery();
  void endQuery();
  void query(VarId);

  void beginCommit();
  void endCommit();

  size_t getNumVariables();
  size_t getNumInvariants();

  [[nodiscard]] const std::vector<VarIdBase>& getDecisionVariables();
  [[nodiscard]] const std::vector<VarIdBase>& getOutputVariables();
  [[nodiscard]] const std::vector<VarIdBase>& getInputVariables(InvariantId);

  /**
   * returns the next dependency at the current timestamp.
   */
  VarId getNextDependency(InvariantId);

  InvariantId getDefiningInvariant(VarId);

  // This function is used by propagation, which is unaware of views.
  [[nodiscard]] inline bool hasChanged(Timestamp t, VarId v) const;

  [[nodiscard]] const std::vector<VarIdBase>& getVariablesDefinedBy(
      InvariantId) const;

  [[nodiscard]] const std::vector<InvariantId>& getListeningInvariants(
      VarId) const;

  /**
   * Notify an invariant that its current dependency has changed
   */
  void notifyCurrentDependencyChanged(InvariantId);

  /**
   * Register that Invariant to depends on variable from depends on dependency
   * @param dependent the invariant that the variable depends on
   * @param source the depending variable
   * @param localId the id of the depending variable in the invariant
   * @param data additional data
   */
  void registerInvariantDependsOnVar(InvariantId dependent, VarId source,
                                     LocalId localId) override;

  void registerVar(VarId) override;
  void registerInvariant(InvariantId) override;

  PropagationGraph& getPropGraph();
};

inline size_t PropagationEngine::getNumVariables() {
  return m_propGraph.getNumVariables();
}

inline size_t PropagationEngine::getNumInvariants() {
  return m_propGraph.getNumInvariants();
}

inline InvariantId PropagationEngine::getDefiningInvariant(VarId v) {
  // Returns NULL_ID is not defined.
  return m_propGraph.getDefiningInvariant(v);
}

inline void PropagationEngine::clearPropagationPath() {
  m_varIsOnPropagationPath.assign_all(false);
}

inline bool PropagationEngine::isOnPropagationPath(VarId id) {
  return m_varIsOnPropagationPath.get(id);
}

inline const std::vector<VarIdBase>& PropagationEngine::getVariablesDefinedBy(
    InvariantId inv) const {
  return m_propGraph.getVariablesDefinedBy(inv);
}

inline const std::vector<InvariantId>& PropagationEngine::getListeningInvariants(
    VarId id) const {
  return m_propGraph.getListeningInvariants(id);
}

inline VarId PropagationEngine::getNextDependency(InvariantId inv) {
  return getSourceId(
      m_store.getInvariant(inv).getNextDependency(m_currentTime, *this));
}
inline void PropagationEngine::notifyCurrentDependencyChanged(InvariantId inv) {
  m_store.getInvariant(inv).notifyCurrentDependencyChanged(m_currentTime,
                                                           *this);
}

inline bool PropagationEngine::hasChanged(Timestamp t, VarId v) const {
  assert(v.idType != VarIdType::view);
  return m_store.getConstIntVar(v).hasChanged(t);
}

inline void PropagationEngine::setValue(Timestamp t, VarId v, Int val) {
  assert(v.idType != VarIdType::view);
  if (!m_propGraph.isDecisionVar(v)) {
    throw VariableIsNotDecisionVariable();
  }
  m_store.getIntVar(v).setValue(t, val);
  notifyMaybeChanged(t, v);
}

inline void PropagationEngine::setPropagationMode(PropagationEngine::PropagationMode m) {
  if (!m_isOpen) {
    throw ModelNotOpenException("Cannot set propagation mode when model is closed");
  }
  m_mode = m;
}

inline PropagationEngine::PropagationMode PropagationEngine::getPropagationMode() {
  return m_mode;
}

inline const std::vector<VarIdBase>& PropagationEngine::getDecisionVariables() {
  return m_propGraph.m_decisionVariables;
}

inline const std::vector<VarIdBase>& PropagationEngine::getOutputVariables() {
  return m_propGraph.m_outputVariables;
}

inline const std::vector<VarIdBase>& PropagationEngine::getInputVariables(InvariantId inv) {
  return m_propGraph.getInputVariables(inv);
}

template <bool DoCommit>
inline void PropagationEngine::outputToInputePropagate() {
  m_outputToInputExplorer.propagate<DoCommit>(m_currentTime);
}
