#pragma once

#include <cassert>
#include <vector>

#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/misc/logging.hpp"
#include "atlantis/propagation/utils/idMap.hpp"
#include "atlantis/types.hpp"
#include "store/store.hpp"

namespace atlantis::propagation {

class IntVar;
class IntView;
class Invariant;
class ViolationInvariant;

class SolverBase {
 protected:
  enum class SolverState { IDLE, PROBE, MOVE, COMMIT, PROCESSING };

  static const size_t ESTIMATED_NUM_OBJECTS = 1;

  Timestamp _currentTimestamp;

  bool _isOpen = true;

  SolverState _solverState = SolverState::IDLE;

  Store _store;

  void incValue(Timestamp, VarId, Int inc);
  inline void incValue(VarId id, Int val) {
    incValue(_currentTimestamp, id, val);
  }

  void updateValue(Timestamp, VarId, Int val);

  inline void updateValue(VarId id, Int val) {
    updateValue(_currentTimestamp, id, val);
  }

  inline bool hasChanged(Timestamp, VarId);

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param definedVarId the variable that is defined by the invariant
   * @param invariantId the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  virtual void registerDefinedVar(VarId definedVarId,
                                  InvariantId invariantId) = 0;

  friend class Invariant;

 public:
  SolverBase(/* args */);

  virtual ~SolverBase() = default;

  virtual void open() = 0;
  virtual void close() = 0;
  virtual void computeBounds() = 0;

  [[nodiscard]] inline bool isOpen() const noexcept { return _isOpen; }
  [[nodiscard]] inline bool isMoving() const noexcept {
    return _solverState == SolverState::MOVE;
  }

  //--------------------- Variable ---------------------

  [[nodiscard]] inline VarId sourceId(VarId id) const {
    return _store.sourceId(id);
  }

  virtual void enqueueComputedVar(VarId) = 0;

  [[nodiscard]] Int value(Timestamp, VarId);
  [[nodiscard]] inline Int currentValue(VarId id) {
    return value(_currentTimestamp, id);
  }

  [[nodiscard]] Int committedValue(VarId);

  [[nodiscard]] Timestamp tmpTimestamp(VarId) const;

  [[nodiscard]] bool isPostponed(InvariantId) const;

  void recompute(InvariantId);
  void recompute(Timestamp, InvariantId);

  void commit(VarId);  // todo: this feels dangerous, maybe commit should
                       // always have a timestamp?
  void commitIf(Timestamp, VarId);
  void commitValue(VarId, Int val);

  [[nodiscard]] inline Int lowerBound(VarId id) const {
    if (id.idType == VarIdType::view) {
      return intViewLowerBound(id);
    }
    assert(id.idType == VarIdType::var);
    return _store.constIntVar(id).lowerBound();
  }

  [[nodiscard]] inline Int intViewLowerBound(VarId id) const {
    assert(id.idType == VarIdType::view);
    return _store.constIntView(id).lowerBound();
  }

  [[nodiscard]] inline Int upperBound(VarId id) const {
    if (id.idType == VarIdType::view) {
      return intViewUpperBound(id);
    }
    assert(id.idType == VarIdType::var);
    return _store.constIntVar(id).upperBound();
  }

  inline void updateBounds(VarId id, Int lb, Int ub, bool widenOnly) {
    assert(id.idType == VarIdType::var);
    _store.intVar(id).updateBounds(lb, ub, widenOnly);
  }

  [[nodiscard]] inline Int intViewUpperBound(VarId id) const {
    assert(id.idType == VarIdType::view);
    return _store.constIntView(id).upperBound();
  }

  void commitInvariantIf(Timestamp, InvariantId);

  void commitInvariant(InvariantId);

  //--------------------- Registration ---------------------
  /**
   * Register an invariant in the solver and return its pointer.
   * This also sets the id of the invariant to the new id.
   * @param args the constructor arguments of the invariant
   * @return the created invariant.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<Invariant, T>::value, T&> makeInvariant(
      Args&&... args);

  /**
   * Register an IntView in the solver and return its pointer.
   * This also sets the id of the IntView to the new id.
   * @param args the constructor arguments of the IntView
   * @return the created IntView.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<IntView, T>::value, VarId> makeIntView(
      Args&&... args);

  /**
   * Register a violation invariant in the solver and return its pointer.
   * This also sets the id of the violation invariant to the new id.
   * @param args the constructor arguments of the violation invariant
   * @return the created violation invariant.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<ViolationInvariant, T>::value, T&>
  makeViolationInvariant(Args&&... args);

  /**
   * Creates an IntVar and registers it to the solver.
   * @return the created IntVar
   */
  VarId makeIntVar(Int initValue, Int lowerBound, Int upperBound);

  /**
   * Register that a variable is a input to an invariant.
   * @param invariantId the invariant
   * @param varId the input
   * @param localId the id of the input in the invariant
   */
  virtual void registerInvariantInput(InvariantId invariantId, VarId varId,
                                      LocalId localId, bool isDynamic) = 0;

  virtual void registerVar(VarId) = 0;
  virtual void registerInvariant(InvariantId) = 0;

  const Store& store();
  [[nodiscard]] Timestamp currentTimestamp() const;
};

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<Invariant, T>::value, T&>
SolverBase::makeInvariant(Args&&... args) {
  if (!_isOpen) {
    throw SolverClosedException("Cannot make invariant when store is closed.");
  }
  const InvariantId invariantId = _store.createInvariantFromPtr(
      std::make_unique<T>(std::forward<Args>(args)...));
  registerInvariant(invariantId);

  logDebug("Created new invariant with id: " << invariantId);
  T& invariant = static_cast<T&>(_store.invariant(invariantId));
  invariant.registerVars();
  invariant.updateBounds(false);
  return invariant;
}

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<IntView, T>::value, VarId>
SolverBase::makeIntView(Args&&... args) {
  if (!_isOpen) {
    throw SolverClosedException("Cannot make intView when store is closed.");
  }
  // We don't actually register views as they are invisible to propagation.

  const VarId viewId = _store.createIntViewFromPtr(
      std::make_unique<T>(std::forward<Args>(args)...));
  _store.intView(viewId).init(viewId);
  return viewId;
}

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<ViolationInvariant, T>::value, T&>
SolverBase::makeViolationInvariant(Args&&... args) {
  if (!_isOpen) {
    throw SolverClosedException("Cannot make invariant when store is closed.");
  }
  const InvariantId violationInvId = _store.createInvariantFromPtr(
      std::make_unique<T>(std::forward<Args>(args)...));
  T& violationInvariant = static_cast<T&>(_store.invariant(violationInvId));
  // A violation invariant is a type of invariant:
  registerInvariant(violationInvId);
  logDebug("Created new Violation Invariant with id: " << violationInvId);
  violationInvariant.registerVars();
  violationInvariant.updateBounds(false);
  return violationInvariant;
}

//--------------------- Inlined functions ---------------------

inline const Store& SolverBase::store() { return _store; }
inline Timestamp SolverBase::currentTimestamp() const {
  return _currentTimestamp;
}

inline bool SolverBase::hasChanged(Timestamp ts, VarId id) {
  return _store.intVar(id).hasChanged(ts);
}

inline Int SolverBase::value(Timestamp ts, VarId id) {
  if (id.idType == VarIdType::view) {
    return _store.intView(id).value(ts);
  }
  return _store.constIntVar(id).value(ts);
}

inline Int SolverBase::committedValue(VarId id) {
  if (id.idType == VarIdType::view) {
    return _store.intView(id).committedValue();
  }
  return _store.constIntVar(id).committedValue();
}

inline Timestamp SolverBase::tmpTimestamp(VarId id) const {
  if (id.idType == VarIdType::view) {
    return _store.constIntVar(_store.intViewSourceId(id)).tmpTimestamp();
  }
  return _store.constIntVar(id).tmpTimestamp();
}

inline bool SolverBase::isPostponed(InvariantId invariantId) const {
  return _store.constInvariant(invariantId).isPostponed();
}

inline void SolverBase::recompute(InvariantId invariantId) {
  return _store.invariant(invariantId).recompute(_currentTimestamp);
}

inline void SolverBase::recompute(Timestamp ts, InvariantId invariantId) {
  return _store.invariant(invariantId).recompute(ts);
}

inline void SolverBase::updateValue(Timestamp ts, VarId id, Int val) {
  _store.intVar(id).setValue(ts, val);
}

inline void SolverBase::incValue(Timestamp ts, VarId id, Int inc) {
  _store.intVar(id).incValue(ts, inc);
}

inline void SolverBase::commit(VarId id) { _store.intVar(id).commit(); }

inline void SolverBase::commitIf(Timestamp ts, VarId id) {
  _store.intVar(id).commitIf(ts);
}

inline void SolverBase::commitValue(VarId id, Int val) {
  _store.intVar(id).commitValue(val);
}

inline void SolverBase::commitInvariantIf(Timestamp ts,
                                          InvariantId invariantId) {
  _store.invariant(invariantId).commit(ts);
}

inline void SolverBase::commitInvariant(InvariantId invariantId) {
  _store.invariant(invariantId).commit(_currentTimestamp);
}

}  // namespace atlantis::propagation
