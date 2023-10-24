#pragma once

#include <deque>
#include <iostream>
#include <queue>
#include <set>

#include "exceptions/exceptions.hpp"
#include "propagation/propagation/outputToInputExplorer.hpp"
#include "propagation/propagation/propagationGraph.hpp"
#include "propagation/solverBase.hpp"
#include "propagation/utils/idMap.hpp"
#include "propagation/variables/intVar.hpp"
#include "utils/hashes.hpp"

namespace atlantis::propagation {

class Solver : public SolverBase {
 protected:
  PropagationMode _propagationMode;

 protected:
  size_t _numVars{0};

  PropagationGraph _propGraph;
  OutputToInputExplorer _outputToInputExplorer;

  IdMap<VarIdBase, bool> _isEnqueued;
  std::vector<std::vector<VarIdBase>> _layerQueue{};
  std::vector<size_t> _layerQueueIndex{};

  std::unordered_set<VarIdBase> _modifiedSearchVars;

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
  virtual void registerDefinedVar(VarId definedVarId,
                                  InvariantId invariantId) final;

 public:
  Solver(/* args */);

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

  size_t numVars() const;
  size_t numInvariants() const;

  [[nodiscard]] const std::vector<VarIdBase>& searchVars() const;
  [[nodiscard]] const std::unordered_set<VarIdBase>& modifiedSearchVar() const;
  [[nodiscard]] const std::vector<VarIdBase>& evaluationVars() const;
  [[nodiscard]] const std::vector<std::pair<VarIdBase, bool>>& inputVars(
      InvariantId) const;

  /**
   * returns the next input at the current timestamp.
   */
  VarId nextInput(InvariantId);

  InvariantId definingInvariant(VarId) const;

  // This function is used by propagation, which is unaware of views.
  [[nodiscard]] inline bool hasChanged(Timestamp, VarId) const;

  [[nodiscard]] const std::vector<VarIdBase>& varsDefinedBy(InvariantId) const;

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

inline void Solver::incCurrentTimestamp() {
  ++_currentTimestamp;
  if (_propagationMode == PropagationMode::INPUT_TO_OUTPUT) {
    clearPropagationQueue();
  } else {
    _modifiedSearchVars.clear();
  }
  assert(std::all_of(
      searchVars().begin(), searchVars().end(), [&](const VarIdBase varId) {
        return !_store.intVar(varId).hasChanged(_currentTimestamp);
      }));
}

inline size_t Solver::numVars() const { return _propGraph.numVars(); }

inline size_t Solver::numInvariants() const {
  return _propGraph.numInvariants();
}

inline InvariantId Solver::definingInvariant(VarId id) const {
  // Returns NULL_ID if there is no defining invariant
  return _propGraph.definingInvariant(id);
}

inline const std::vector<VarIdBase>& Solver::varsDefinedBy(
    InvariantId invariantId) const {
  return _propGraph.varsDefinedBy(invariantId);
}

inline const std::vector<PropagationGraph::ListeningInvariantData>&
Solver::listeningInvariantData(VarId id) const {
  return _propGraph.listeningInvariantData(id);
}

inline VarId Solver::nextInput(InvariantId invariantId) {
  return sourceId(_store.invariant(invariantId).nextInput(_currentTimestamp));
}
inline void Solver::notifyCurrentInputChanged(InvariantId invariantId) {
  _store.invariant(invariantId).notifyCurrentInputChanged(_currentTimestamp);
}

inline bool Solver::hasChanged(Timestamp ts, VarId id) const {
  assert(id.idType != VarIdType::view);
  return _store.constIntVar(id).hasChanged(ts);
}

inline void Solver::setValue(Timestamp ts, VarId id, Int val) {
  assert(id.idType != VarIdType::view);
  assert(_propGraph.isSearchVar(id));

  IntVar& var = _store.intVar(id);
  var.setValue(ts, val);

  if (_propagationMode == PropagationMode::OUTPUT_TO_INPUT) {
    if (ts != _currentTimestamp) {
      _modifiedSearchVars.clear();
    }

    if (var.hasChanged(ts)) {
      _modifiedSearchVars.emplace(id);
    } else {
      _modifiedSearchVars.erase(id);
    }
  }
  enqueueComputedVar(id);
}

inline void Solver::setPropagationMode(PropagationMode propMode) {
  if (!_isOpen) {
    throw SolverClosedException(
        "Cannot set propagation mode when model is closed");
  }
  _propagationMode = propMode;
}

inline OutputToInputMarkingMode Solver::outputToInputMarkingMode() const {
  return _outputToInputExplorer.outputToInputMarkingMode();
}

inline void Solver::setOutputToInputMarkingMode(
    OutputToInputMarkingMode markingMode) {
  if (!_isOpen) {
    throw SolverClosedException(
        "Cannot set output-to-input marking mode when model is closed");
  }
  _outputToInputExplorer.setOutputToInputMarkingMode(markingMode);
}

inline const std::vector<VarIdBase>& Solver::searchVars() const {
  return _propGraph.searchVars();
}

inline const std::vector<VarIdBase>& Solver::evaluationVars() const {
  return _propGraph.evaluationVars();
}

inline const std::vector<std::pair<VarIdBase, bool>>& Solver::inputVars(
    InvariantId invariantId) const {
  return _propGraph.inputVars(invariantId);
}

inline const std::unordered_set<VarIdBase>& Solver::modifiedSearchVar() const {
  return _modifiedSearchVars;
}

inline void Solver::outputToInputPropagate() {
  assert(propagationMode() == PropagationMode::OUTPUT_TO_INPUT);
  _outputToInputExplorer.propagate(_currentTimestamp);
}

}  // namespace atlantis::propagation