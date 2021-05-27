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
  static const size_t ESTIMATED_NUM_OBJECTS = 1;

  Timestamp _currentTimestamp;

  bool _isOpen = true;
  bool _isMoving = false;

  struct ListeningInvariantData {
    InvariantId id;
    LocalId localId;
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
  inline bool isMoving() const noexcept { return _isMoving; }

  //--------------------- Variable ---------------------

  inline VarId sourceId(VarId id) {
    return id.idType == VarIdType::var ? id : _store.intViewSourceId(id);
    // sourceId(_store.intView(id).parantId());
  }

  virtual void queueForPropagation(Timestamp, VarId) = 0;
  virtual void notifyMaybeChanged(Timestamp, VarId) = 0;

  Int value(Timestamp, VarId);
  inline Int newValue(VarId id) { return value(_currentTimestamp, id); }

  Int intViewValue(Timestamp, VarId);

  Int committedValue(VarId);

  Int intViewCommittedValue(VarId);

  Timestamp tmpTimestamp(VarId);

  bool isPostponed(InvariantId);

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
    return _store.constIntView(id)->lowerBound();
  }

  [[nodiscard]] inline Int upperBound(VarId id) const {
    if (id.idType == VarIdType::view) {
      return intViewUpperBound(id);
    }
    assert(id.idType == VarIdType::var);
    return _store.constIntVar(id).upperBound();
  }

  [[nodiscard]] inline Int intViewUpperBound(VarId id) const {
    assert(id.idType == VarIdType::view);
    return _store.constIntView(id)->upperBound();
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
  std::enable_if_t<std::is_base_of<Invariant, T>::value, std::shared_ptr<T>>
  makeInvariant(Args&&... args);

  /**
   * Register an IntView in the engine and return its pointer.
   * This also sets the id of the IntView to the new id.
   * @param args the constructor arguments of the IntView
   * @return the created IntView.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<IntView, T>::value, std::shared_ptr<T>>
  makeIntView(Args&&... args);

  /**
   * Register a constraint in the engine and return its pointer.
   * This also sets the id of the constraint to the new id.
   * @param args the constructor arguments of the constraint
   * @return the created constraint.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<Constraint, T>::value, std::shared_ptr<T>>
  makeConstraint(Args&&... args);

  /**
   * Creates an IntVar and registers it to the engine.
   * @return the created IntVar
   */
  VarId makeIntVar(Int initValue, Int lowerBound, Int upperBound);

  /**
   * Register that a variable is a parameter to an invariant.
   * @param invariantId the invariant
   * @param varId the parameter
   * @param localId the id of the parameter in the invariant
   */
  virtual void registerInvariantParameter(InvariantId invariantId, VarId varId,
                                          LocalId localId) = 0;

  virtual void registerVar(VarId) = 0;
  virtual void registerInvariant(InvariantId) = 0;

  const Store& store();
  [[nodiscard]] Timestamp currentTimestamp() const;
};

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<Invariant, T>::value, std::shared_ptr<T>>
Engine::makeInvariant(Args&&... args) {
  if (!_isOpen) {
    throw ModelNotOpenException("Cannot make invariant when store is closed.");
  }
  auto invariantPtr = std::make_shared<T>(std::forward<Args>(args)...);

  auto newId = _store.createInvariantFromPtr(invariantPtr);
  registerInvariant(newId);
  logDebug("Created new invariant with id: " << newId);
  invariantPtr->init(_currentTimestamp, *this);
  return invariantPtr;
}

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<IntView, T>::value, std::shared_ptr<T>>
Engine::makeIntView(Args&&... args) {
  if (!_isOpen) {
    throw ModelNotOpenException("Cannot make intView when store is closed.");
  }
  auto viewPtr = std::make_shared<T>(std::forward<Args>(args)...);

  auto newId = _store.createIntViewFromPtr(viewPtr);
  // We don'ts actually register views as they are invisible to propagation.

  viewPtr->init(newId, *this);
  return viewPtr;
}

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<Constraint, T>::value, std::shared_ptr<T>>
Engine::makeConstraint(Args&&... args) {
  if (!_isOpen) {
    throw ModelNotOpenException("Cannot make invariant when store is closed.");
  }
  auto constraintPtr = std::make_shared<T>(std::forward<Args>(args)...);

  auto newId = _store.createInvariantFromPtr(constraintPtr);
  registerInvariant(newId);  // A constraint is a type of invariant.
  logDebug("Created new Constraint with id: " << newId);
  constraintPtr->init(_currentTimestamp, *this);
  return constraintPtr;
}

//--------------------- Inlined functions ---------------------

inline const Store& Engine::store() { return _store; }
inline Timestamp Engine::currentTimestamp() const { return _currentTimestamp; }

inline bool Engine::hasChanged(Timestamp ts, VarId id) {
  return _store.intVar(id).hasChanged(ts);
}

inline Int Engine::value(Timestamp ts, VarId id) {
  if (id.idType == VarIdType::view) {
    return intViewValue(ts, id);
  }
  return _store.intVar(id).value(ts);
}

inline Int Engine::intViewValue(Timestamp ts, VarId id) {
  return _store.intView(id).value(ts);
}

inline Int Engine::committedValue(VarId id) {
  if (id.idType == VarIdType::view) {
    return intViewCommittedValue(id);
  }
  return _store.intVar(id).committedValue();
}

inline Int Engine::intViewCommittedValue(VarId id) {
  return _store.intView(id).committedValue();
}

inline Timestamp Engine::tmpTimestamp(VarId id) {
  if (id.idType == VarIdType::view) {
    return _store.intVar(_store.intViewSourceId(id)).tmpTimestamp();
  }
  return _store.intVar(id).tmpTimestamp();
}

inline bool Engine::isPostponed(InvariantId id) {
  return _store.invariant(id).isPostponed();
}

inline void Engine::recompute(InvariantId id) {
  return _store.invariant(id).recompute(_currentTimestamp, *this);
}

inline void Engine::recompute(Timestamp ts, InvariantId id) {
  return _store.invariant(id).recompute(ts, *this);
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

inline void Engine::commitInvariantIf(Timestamp ts, InvariantId id) {
  _store.invariant(id).commit(ts, *this);
}

inline void Engine::commitInvariant(InvariantId id) {
  _store.invariant(id).commit(_currentTimestamp, *this);
}