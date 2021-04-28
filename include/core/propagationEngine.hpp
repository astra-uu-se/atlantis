#pragma once

#include <queue>

#include "core/engine.hpp"
#include "core/idMap.hpp"
#include "misc/logging.hpp"
#include "propagation/bottomUpExplorer.hpp"
#include "propagation/propagationGraph.hpp"

class PropagationEngine : public Engine {
 public:
  enum class PropagationMode { TOP_DOWN, BOTTOM_UP, MIXED };
  PropagationMode mode;
  const bool m_useMarkingForBottomUp = true;

 protected:
  size_t m_numVariables;

  PropagationGraph m_propGraph;
  BottomUpExplorer m_bottomUpExplorer;

  IdMap<VarId, bool> m_isEnqueued;

  IdMap<VarId, bool> m_varIsOnPropagationPath;
  std::queue<VarId> m_propagationPathQueue;

  void recomputeAndCommit();

  void emptyModifiedVariables();

  template <bool DoCommit>
  void propagate();

  template <bool DoCommit>
  void bottomUpPropagate();

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

  /**
   * returns the next dependency at the current timestamp.
   */
  VarId getNextDependency(InvariantId);

  InvariantId getDefiningInvariant(VarId);

  // This function is used by propagation, which is unaware of views.
  [[nodiscard]] inline bool hasChanged(Timestamp t, VarId v) const;

  [[nodiscard]] const std::vector<VarIdBase>& getVariablesDefinedBy(
      InvariantId) const;

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

inline InvariantId PropagationEngine::getDefiningInvariant(VarId v) {
  // Returns NULL_ID is not defined.
  return m_propGraph.getDefiningInvariant(v);
}

inline void PropagationEngine::clearPropagationPath() {
  m_varIsOnPropagationPath.assign_all(false);
}

inline bool PropagationEngine::isOnPropagationPath(VarId id) {
  return !m_useMarkingForBottomUp || m_varIsOnPropagationPath.get(id);
}

inline const std::vector<VarIdBase>& PropagationEngine::getVariablesDefinedBy(
    InvariantId inv) const {
  return m_propGraph.getVariablesDefinedBy(inv);
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
  m_store.getIntVar(v).setValue(t, val);
  notifyMaybeChanged(t, v);
}

inline void PropagationEngine::setPropagationMode(PropagationMode m) {
  assert(m_isOpen);
  mode = m;
}

// Propagates at the current internal time of the engine.
template <bool DoCommit>
void PropagationEngine::propagate() {
//#define PROPAGATION_DEBUG
// #define PROPAGATION_DEBUG_COUNTING
#ifdef PROPAGATION_DEBUG
  setLogLevel(debug);
  logDebug("Starting propagation");
#endif
#ifdef PROPAGATION_DEBUG_COUNTING
  std::vector<std::unordered_map<size_t, Int>> notificationCount(
      m_store.getNumInvariants());
#endif

  for (VarId id = getNextStableVariable(m_currentTime); id.id != NULL_ID;
       id = getNextStableVariable(m_currentTime)) {
    IntVar& variable = m_store.getIntVar(id);

    InvariantId definingInvariant = m_propGraph.getDefiningInvariant(id);

#ifdef PROPAGATION_DEBUG
    logDebug("\tPropagating " << variable);
    logDebug("\t\tDepends on invariant: " << definingInvariant);
#endif

    if (definingInvariant != NULL_ID) {
      Invariant& defInv = m_store.getInvariant(definingInvariant);
      if (id == defInv.getPrimaryOutput()) {
        Int oldValue = variable.getValue(m_currentTime);
        defInv.compute(m_currentTime, *this);
        defInv.queueNonPrimaryOutputVarsForPropagation(m_currentTime, *this);
        if (oldValue == variable.getValue(m_currentTime)) {
#ifdef PROPAGATION_DEBUG
          logDebug("\t\tVariable did not change after compute: ignoring.");
#endif

          continue;
        }
        if constexpr (DoCommit) {
          defInv.commit(m_currentTime, *this);
        }
      }
      if constexpr (DoCommit) {
        commitIf(m_currentTime, id);
      }
    }

    for (auto& toNotify : m_dependentInvariantData[id]) {
      Invariant& invariant = m_store.getInvariant(toNotify.id);

#ifdef PROPAGATION_DEBUG
      logDebug("\t\tNotifying invariant:" << toNotify.id << " with localId: "
                                          << toNotify.localId);
#endif
#ifdef PROPAGATION_DEBUG_COUNTING
      notificationCount.at(toNotify.id.id - 1)[variable.m_id.id] =
          notificationCount.at(toNotify.id.id - 1)[variable.m_id.id] + 1;
#endif

      invariant.notify(toNotify.localId);
      queueForPropagation(m_currentTime, invariant.getPrimaryOutput());
    }
  }

#ifdef PROPAGATION_DEBUG_COUNTING
  logDebug("Printing notification counts");
  for (int i = 0; i < notificationCount.size(); ++i) {
    logDebug("\tInvariant " << i + 1);
    for (auto [k, v] : notificationCount.at(i)) {
      logDebug("\t\tVarId(" << k << "): " << v);
    }
  }
#endif
#ifdef PROPAGATION_DEBUG
  logDebug("Propagation done\n");
#endif
}

template <bool DoCommit>
inline void PropagationEngine::bottomUpPropagate() {
  m_bottomUpExplorer.propagate<DoCommit>(m_currentTime);
}

