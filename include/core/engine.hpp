#pragma once

#include <cassert>
#include <vector>

#include "core/types.hpp"
#include "exceptions/exceptions.hpp"
#include "misc/logging.hpp"
#include "store/store.hpp"
#include "utils/idMap.hpp"

class IntVar;
class IntView;
class Invariant;
class Constraint;

class Engine {
 protected:
  enum class EngineState { IDLE, PROBE, MOVE, COMMIT, PROCESSING };

  static const size_t ESTIMATED_NUM_OBJECTS = 1;

  Timestamp _currentTimestamp;

  bool _isOpen = true;

  EngineState _engineState = EngineState::IDLE;
  struct ListeningInvariantData {
    const InvariantId invariantId;
    const LocalId localId;
  };
  // Map from VarID -> vector of InvariantID
  IdMap<VarIdBase, std::vector<ListeningInvariantData>> _listeningInvariantData;

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
  virtual void registerDefinedVariable(VarId definedVarId,
                                       InvariantId invariantId) = 0;

  friend class Invariant;

 public:
  Engine(/* args */);

  virtual ~Engine() = default;

  virtual void open() = 0;
  virtual void close() = 0;
  virtual void computeBounds() = 0;

  [[nodiscard]] inline bool isOpen() const noexcept { return _isOpen; }
  [[nodiscard]] inline bool isMoving() const noexcept {
    return _engineState == EngineState::MOVE;
  }

  //--------------------- Variable ---------------------

  [[nodiscard]] inline VarId sourceId(VarId id) const {
    return id.idType == VarIdType::var ? id : _store.intViewSourceId(id);
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
   * Register an invariant in the engine and return its pointer.
   * This also sets the id of the invariant to the new id.
   * @param args the constructor arguments of the invariant
   * @return the created invariant.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<Invariant, T>::value, T&> makeInvariant(
      Args&&... args);

  /**
   * Register an IntView in the engine and return its pointer.
   * This also sets the id of the IntView to the new id.
   * @param args the constructor arguments of the IntView
   * @return the created IntView.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<IntView, T>::value, VarId> makeIntView(
      Args&&... args);

  /**
   * Register a constraint in the engine and return its pointer.
   * This also sets the id of the constraint to the new id.
   * @param args the constructor arguments of the constraint
   * @return the created constraint.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<Constraint, T>::value, T&> makeConstraint(
      Args&&... args);

  /**
   * Creates an IntVar and registers it to the engine.
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
                                      LocalId localId,
                                      bool isDynamic = false) = 0;

  virtual void registerVar(VarId) = 0;
  virtual void registerInvariant(InvariantId) = 0;

  const Store& store();
  [[nodiscard]] Timestamp currentTimestamp() const;
};

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<Invariant, T>::value, T&>
Engine::makeInvariant(Args&&... args) {
  if (!_isOpen) {
    throw EngineClosedException("Cannot make invariant when store is closed.");
  }
  const InvariantId invariantId = _store.createInvariantFromPtr(
      std::make_unique<T>(std::forward<Args>(args)...));
  registerInvariant(invariantId);

  logDebug("Created new invariant with id: " << invariantId);
  T& invariant = static_cast<T&>(_store.invariant(invariantId));
  invariant.registerVars(*this);
  invariant.updateBounds(*this, false);
  return invariant;
}

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<IntView, T>::value, VarId> Engine::makeIntView(
    Args&&... args) {
  if (!_isOpen) {
    throw EngineClosedException("Cannot make intView when store is closed.");
  }
  // We don't actually register views as they are invisible to propagation.

  const VarId viewId = _store.createIntViewFromPtr(
      std::make_unique<T>(std::forward<Args>(args)...));
  _store.intView(viewId).init(viewId, *this);
  return viewId;
}

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<Constraint, T>::value, T&>
Engine::makeConstraint(Args&&... args) {
  if (!_isOpen) {
    throw EngineClosedException("Cannot make invariant when store is closed.");
  }
  const InvariantId constraintId = _store.createInvariantFromPtr(
      std::make_unique<T>(std::forward<Args>(args)...));
  T& constraint = static_cast<T&>(_store.invariant(constraintId));
  registerInvariant(constraintId);  // A constraint is a type of invariant.
  logDebug("Created new Constraint with id: " << constraintId);
  constraint.registerVars(*this);
  constraint.updateBounds(*this);
  return constraint;
}

//--------------------- Inlined functions ---------------------

inline const Store& Engine::store() { return _store; }
inline Timestamp Engine::currentTimestamp() const { return _currentTimestamp; }

inline bool Engine::hasChanged(Timestamp ts, VarId id) {
  return _store.intVar(id).hasChanged(ts);
}

inline Int Engine::value(Timestamp ts, VarId id) {
  if (id.idType == VarIdType::view) {
    return _store.intView(id).value(ts);
  }
  return _store.constIntVar(id).value(ts);
}

inline Int Engine::committedValue(VarId id) {
  if (id.idType == VarIdType::view) {
    return _store.intView(id).committedValue();
  }
  return _store.constIntVar(id).committedValue();
}

inline Timestamp Engine::tmpTimestamp(VarId id) const {
  if (id.idType == VarIdType::view) {
    return _store.constIntVar(_store.intViewSourceId(id)).tmpTimestamp();
  }
  return _store.constIntVar(id).tmpTimestamp();
}

inline bool Engine::isPostponed(InvariantId invariantId) const {
  return _store.constInvariant(invariantId).isPostponed();
}

inline void Engine::recompute(InvariantId invariantId) {
  return _store.invariant(invariantId).recompute(_currentTimestamp, *this);
}

inline void Engine::recompute(Timestamp ts, InvariantId invariantId) {
  return _store.invariant(invariantId).recompute(ts, *this);
}

inline void Engine::updateValue(Timestamp ts, VarId id, Int val) {
  _store.intVar(id).setValue(ts, val);
}

inline void Engine::incValue(Timestamp ts, VarId id, Int inc) {
  _store.intVar(id).incValue(ts, inc);
}

inline void Engine::commit(VarId id) { _store.intVar(id).commit(); }

inline void Engine::commitIf(Timestamp ts, VarId id) {
  _store.intVar(id).commitIf(ts);
}

inline void Engine::commitValue(VarId id, Int val) {
  _store.intVar(id).commitValue(val);
}

inline void Engine::commitInvariantIf(Timestamp ts, InvariantId invariantId) {
  _store.invariant(invariantId).commit(ts, *this);
}

inline void Engine::commitInvariant(InvariantId invariantId) {
  _store.invariant(invariantId).commit(_currentTimestamp, *this);
}