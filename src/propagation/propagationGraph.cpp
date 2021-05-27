#include "propagation/propagationGraph.hpp"

#include <algorithm>
#include <iostream>

#include "misc/logging.hpp"

PropagationGraph::PropagationGraph(size_t expectedSize)
    : _numInvariants(0),
      _numVariables(0),
      _definingInvariant(expectedSize),
      _variablesDefinedByInvariant(expectedSize),
      _variableParameters(expectedSize),
      _listeningInvariants(expectedSize),
      _topology(*this) {}

void PropagationGraph::registerInvariant([[maybe_unused]] InvariantId id) {
  // Everything must be registered in sequence.
  _variablesDefinedByInvariant.register_idx(id);
  _variableParameters.register_idx(id);
  ++_numInvariants;
}

void PropagationGraph::registerVar([[maybe_unused]] VarIdBase id) {
  _listeningInvariants.register_idx(id);
  _definingInvariant.register_idx(id);
  ++_numVariables;
}

void PropagationGraph::registerInvariantParameter(InvariantId invariantId,
                                                  VarIdBase varId) {
  assert(!invariantId.equals(NULL_ID) && !varId.equals(NULL_ID));
  if (_definingInvariant[varId] == invariantId) {
    logWarning("The invariant (" << invariantId << ") already "
                                 << "defines the varId variable (" << varId
                                 << "); "
                                 << "ignoring (selft-cyclic) dependency.");
    return;
  }
  _listeningInvariants[varId].push_back(invariantId);
  _variableParameters[invariantId].push_back(varId);
}

void PropagationGraph::registerDefinedVariable(VarIdBase varId,
                                               InvariantId invariantId) {
  assert(!varId.equals(NULL_ID) && !invariantId.equals(NULL_ID));
  if (_definingInvariant.at(varId).id != NULL_ID.id) {
    throw VariableAlreadyDefinedException(
        "Variable " + std::to_string(varId.id) +
        " already defined by invariant " +
        std::to_string(_definingInvariant.at(varId).id));
  }
  Int index = -1;
  for (size_t i = 0; i < _listeningInvariants[varId].size(); ++i) {
    if (_listeningInvariants[varId][i] == invariantId) {
      index = i;
      break;
    }
  }
  if (index >= 0) {
    _listeningInvariants[varId].erase(_listeningInvariants[varId].begin() +
                                      index);
    logWarning("The (self-cyclic) dependency that the invariant "
               << "(" << invariantId << ") depends on the parameter "
               << "variable (" << invariantId << ") was removed.");
  }
  _definingInvariant[varId] = invariantId;
  _variablesDefinedByInvariant[invariantId].push_back(varId);
}

void PropagationGraph::close() {
  _isInputVar.resize(getNumVariables() + 1);
  _isOutputVar.resize(getNumVariables() + 1);
  for (size_t i = 1; i < getNumVariables() + 1; ++i) {
    _isOutputVar.at(i) = (_listeningInvariants.at(i).empty());
    _isInputVar.at(i) = (_definingInvariant.at(i) == NULL_ID);
    if (_isOutputVar.at(i)) {
      _outputVariables.push_back(i);
    }
    if (_isInputVar.at(i)) {
      _inputVariables.push_back(i);
    }
  }

  _topology.computeLayersWithCycles();
  // Reset propagation queue data structure.
  // TODO: Be sure that this does not cause a memeory leak...
  // _propagationQueue = PropagationQueue();
  size_t numLayers = 1 + *std::max_element(_topology.variablePosition.begin(),
                                           _topology.variablePosition.end());
  _propagationQueue.init(getNumVariables(), numLayers);
  for (size_t i = 1; i < getNumVariables() + 1; ++i) {
    VarIdBase id = VarIdBase(i);
    _propagationQueue.initVar(id, _topology.getPosition(id));
  }
  // _topology.computeNoCycles();
}