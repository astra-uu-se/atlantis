#pragma once

#include <unordered_set>
#include <vector>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/propagation/propagation/outputToInputExplorer.hpp"
#include "atlantis/propagation/propagation/propagationGraph.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/utils/hashes.hpp"
#include "atlantis/propagation/variables/intVar.hpp"

namespace atlantis::propagation {

class Solver : public SolverBase {
 protected:
  PropagationMode _propagationMode;

 protected:
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

  template <bool SingleLayer>
  void enforceInvariant(VarId, const var::OutgoingArc&, size_t curLayer);

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
  /***
   * @param id the id of the changed variable
   */
  void enqueueDefinedVar(VarId) final;
  void enqueueDefinedVar(VarId, size_t layer);

  [[nodiscard]] inline PropagationMode propagationMode() const {
    return _propagationMode;
  }

  // --------------------- Activity ----------------
  [[nodiscard]] VarId dequeueComputedVar(Timestamp);

  //--------------------- Propagation ---------------------
  void beginMove();
  void endMove();
  void setValue(Timestamp, VarId, Int val);
  void setValue(Timestamp, VarViewId, Int val);

  inline void setValue(VarId id, Int val) {
    setValue(_currentTimestamp, id, val);
  }

  inline void setValue(VarViewId id, Int val) {
    setValue(_currentTimestamp, id, val);
  }

  void beginProbe();
  void endProbe();
  void query(VarViewId);

  void beginCommit();
  void endCommit();

  size_t numVars() const;
  size_t numInvariants() const;

  [[nodiscard]] const std::vector<VarId>& searchVars() const;
  [[nodiscard]] const std::unordered_set<VarId>& modifiedSearchVar() const;
  [[nodiscard]] const std::vector<VarId>& staticInputVars(InvariantId) const;

  /**
   * returns the next input at the current timestamp.
   */
  VarId nextInput(InvariantId);

  InvariantId definingInvariant(VarViewId) const;

  [[nodiscard]] size_t numInputVars(InvariantId) const;

  // This function is used by propagation, which is unaware of views.
  [[nodiscard]] inline bool hasChanged(Timestamp, VarId) const;

  [[nodiscard]] const std::vector<VarId>& varsDefinedBy(InvariantId) const;

  [[nodiscard]] const var::OutgoingArcContainer& outgoingArcs(VarId) const;

  void commitOutgoingArcs(Timestamp, VarId) const;

  void makeDynamicInputActive(Timestamp ts, InvariantId invId,
                              LocalId localId) override;

  void makeDynamicInputInactive(Timestamp ts, InvariantId invId,
                                LocalId localId) override;

  void makeAllDynamicInputsInactive(Timestamp, InvariantId) override;

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
  LocalId registerInvariantInput(InvariantId invariantId, VarViewId inputId,
                                 bool isDynamic) final;

  void registerVar(VarId) final;
  void registerInvariant(InvariantId) final;
};

inline void Solver::incCurrentTimestamp() {
  ++_currentTimestamp;
  if (_propagationMode == PropagationMode::INPUT_TO_OUTPUT) {
    clearPropagationQueue();
  } else {
    _modifiedSearchVars.clear();
  }
  assert(std::all_of(
      searchVars().begin(), searchVars().end(), [&](const VarId varId) {
        return !_store.intVar(varId).hasChanged(_currentTimestamp);
      }));
}

inline size_t Solver::numVars() const { return _propGraph.numVars(); }

inline size_t Solver::numInvariants() const {
  return _propGraph.numInvariants();
}

inline InvariantId Solver::definingInvariant(VarViewId id) const {
  return _propGraph.definingInvariant(id.isView() ? sourceId(id) : VarId(id));
}

inline const std::vector<VarId>& Solver::varsDefinedBy(
    InvariantId invariantId) const {
  return _propGraph.varsDefinedBy(invariantId);
}

inline const var::OutgoingArcContainer& Solver::outgoingArcs(VarId id) const {
  return _propGraph.outgoingArcs(id);
}

inline size_t Solver::numInputVars(InvariantId invId) const {
  return _propGraph.numInputVars(invId);
}

inline VarId Solver::nextInput(InvariantId invariantId) {
  return sourceId(_store.invariant(invariantId).nextInput(_currentTimestamp));
}
inline void Solver::notifyCurrentInputChanged(InvariantId invariantId) {
  _store.invariant(invariantId).notifyCurrentInputChanged(_currentTimestamp);
}

inline bool Solver::hasChanged(Timestamp ts, VarId id) const {
  return _store.constIntVar(id).hasChanged(ts);
}

inline void Solver::setValue(Timestamp ts, VarViewId id, Int val) {
  assert(id.isVar());
  setValue(ts, VarId(id), val);
}

inline void Solver::setValue(Timestamp ts, VarId id, Int val) {
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
  enqueueDefinedVar(id);
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

inline const std::vector<VarId>& Solver::searchVars() const {
  return _propGraph.searchVars();
}

inline const std::vector<VarId>& Solver::staticInputVars(
    InvariantId invariantId) const {
  return _propGraph.staticInputVars(invariantId);
}

inline const std::unordered_set<VarId>& Solver::modifiedSearchVar() const {
  return _modifiedSearchVars;
}

inline void Solver::outputToInputPropagate() {
  assert(propagationMode() == PropagationMode::OUTPUT_TO_INPUT);
  _outputToInputExplorer.propagate(_currentTimestamp);
}

}  // namespace atlantis::propagation
