#pragma once

#include <cassert>
#include <vector>

// #include "constraints/constraint.hpp"
#include "core/types.hpp"
#include "exceptions/exceptions.hpp"
#include "misc/logging.hpp"
#include "store/store.hpp"
#include "utils/idMap.hpp"
// #include "variables/intVar.hpp"
// #include "views/intView.hpp"

class IntVar;
class IntView;
class Invariant;
class Constraint;

class Engine {
 protected:
  enum class EngineState { IDLE, QUERY, MOVE, COMMIT, PROCESSING };

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

  inline bool isOpen() const noexcept { return _isOpen; }
  inline bool isMoving() const noexcept {
    return _engineState == EngineState::MOVE;
  }

  //--------------------- Variable ---------------------

  inline VarId getSourceId(VarId id) const {
    return id.idType == VarIdType::var ? id : _store.getIntViewSourceId(id);
    // getSourceId(_store.getIntView(id).getParentId());
  }

  virtual void queueForPropagation(Timestamp, VarId) = 0;
  virtual void notifyMaybeChanged(Timestamp, VarId) = 0;

  Int getValue(Timestamp, VarId) const;
  inline Int getNewValue(VarId id) const {
    return getValue(_currentTimestamp, id);
  }

  Int getIntViewValue(Timestamp, VarId) const;

  Int getCommittedValue(VarId) const;

  Int getIntViewCommittedValue(VarId) const;

  Timestamp getTmpTimestamp(VarId) const;

  bool isPostponed(InvariantId) const;

  void recompute(InvariantId);
  void recompute(Timestamp, InvariantId);

  void commit(VarId);  // todo: this feels dangerous, maybe commit should
                       // always have a timestamp?
  void commitIf(Timestamp, VarId);
  void commitValue(VarId, Int val);

  [[nodiscard]] inline Int getLowerBound(VarId id) const {
    if (id.idType == VarIdType::view) {
      return getIntViewLowerBound(id);
    }
    assert(id.idType == VarIdType::var);
    return _store.getConstIntVar(id).getLowerBound();
  }

  [[nodiscard]] inline Int getIntViewLowerBound(VarId id) const {
    assert(id.idType == VarIdType::view);
    return _store.getConstIntView(id).getLowerBound();
  }

  [[nodiscard]] inline Int getUpperBound(VarId id) const {
    if (id.idType == VarIdType::view) {
      return getIntViewUpperBound(id);
    }
    assert(id.idType == VarIdType::var);
    return _store.getConstIntVar(id).getUpperBound();
  }

  [[nodiscard]] inline Int getIntViewUpperBound(VarId id) const {
    assert(id.idType == VarIdType::view);
    return _store.getConstIntView(id).getUpperBound();
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
                                      LocalId localId) = 0;

  virtual void registerVar(VarId) = 0;
  virtual void registerInvariant(InvariantId) = 0;

  const Store& getStore();
  [[nodiscard]] Timestamp getCurrentTimestamp() const;
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
  T& invariant = static_cast<T&>(_store.getInvariant(invariantId));
  invariant.init(_currentTimestamp, *this);
  return invariant;
}

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<IntView, T>::value, VarId> Engine::makeIntView(
    Args&&... args) {
  if (!_isOpen) {
    throw EngineClosedException("Cannot make intView when store is closed.");
  }
  const auto viewPtr = std::make_shared<T>(std::forward<Args>(args)...);

  // We don'ts actually register views as they are invisible to propagation.

  const VarId viewId = _store.createIntViewFromPtr(
      std::make_unique<T>(std::forward<Args>(args)...));
  _store.getIntView(viewId).init(viewId, *this);
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
  T& constraint = static_cast<T&>(_store.getInvariant(constraintId));
  registerInvariant(constraintId);  // A constraint is a type of invariant.
  logDebug("Created new Constraint with id: " << constraintId);
  constraint.init(_currentTimestamp, *this);
  return constraint;
}

//--------------------- Inlined functions ---------------------

inline const Store& Engine::getStore() { return _store; }
inline Timestamp Engine::getCurrentTimestamp() const {
  return _currentTimestamp;
}

inline bool Engine::hasChanged(Timestamp ts, VarId id) {
  return _store.getIntVar(id).hasChanged(ts);
}

inline Int Engine::getValue(Timestamp ts, VarId id) const {
  if (id.idType == VarIdType::view) {
    return getIntViewValue(ts, id);
  }
  return _store.getConstIntVar(id).getValue(ts);
}

inline Int Engine::getIntViewValue(Timestamp ts, VarId id) const {
  return _store.getConstIntView(id).getValue(ts);
}

inline Int Engine::getCommittedValue(VarId id) const {
  if (id.idType == VarIdType::view) {
    return getIntViewCommittedValue(id);
  }
  return _store.getConstIntVar(id).getCommittedValue();
}

inline Int Engine::getIntViewCommittedValue(VarId id) const {
  return _store.getConstIntView(id).getCommittedValue();
}

inline Timestamp Engine::getTmpTimestamp(VarId id) const {
  if (id.idType == VarIdType::view) {
    return _store.getConstIntVar(_store.getIntViewSourceId(id))
        .getTmpTimestamp();
  }
  return _store.getConstIntVar(id).getTmpTimestamp();
}

inline bool Engine::isPostponed(InvariantId invariantId) const {
  return _store.getConstInvariant(invariantId).isPostponed();
}

inline void Engine::recompute(InvariantId invariantId) {
  return _store.getInvariant(invariantId).recompute(_currentTimestamp, *this);
}

inline void Engine::recompute(Timestamp ts, InvariantId invariantId) {
  return _store.getInvariant(invariantId).recompute(ts, *this);
}

inline void Engine::updateValue(Timestamp ts, VarId id, Int val) {
  _store.getIntVar(id).setValue(ts, val);
}

inline void Engine::incValue(Timestamp ts, VarId id, Int inc) {
  _store.getIntVar(id).incValue(ts, inc);
}

inline void Engine::commit(VarId id) { _store.getIntVar(id).commit(); }

inline void Engine::commitIf(Timestamp ts, VarId id) {
  _store.getIntVar(id).commitIf(ts);
}

inline void Engine::commitValue(VarId id, Int val) {
  _store.getIntVar(id).commitValue(val);
}

inline void Engine::commitInvariantIf(Timestamp ts, InvariantId invariantId) {
  _store.getInvariant(invariantId).commit(ts, *this);
}

inline void Engine::commitInvariant(InvariantId invariantId) {
  _store.getInvariant(invariantId).commit(_currentTimestamp, *this);
}