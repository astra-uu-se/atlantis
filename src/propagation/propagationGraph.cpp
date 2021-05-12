#include "propagation/propagationGraph.hpp"

#include <algorithm>
#include <iostream>

#include "misc/logging.hpp"

PropagationGraph::PropagationGraph(size_t expectedSize)
    : _numInvariants(0),
      _numVariables(0),
      _definingInvariant(expectedSize),
      _variablesDefinedByInvariant(expectedSize),
      _inputVariables(expectedSize),
      _listeningInvariants(expectedSize),
      _topology(*this) {}

void PropagationGraph::registerInvariant([[maybe_unused]] InvariantId id) {
  // Everything must be registered in sequence.
  _variablesDefinedByInvariant.register_idx(id);
  _inputVariables.register_idx(id);
  ++_numInvariants;
}

void PropagationGraph::registerVar([[maybe_unused]] VarIdBase id) {
  _listeningInvariants.register_idx(id);
  _definingInvariant.register_idx(id);
  ++_numVariables;
}

void PropagationGraph::registerInvariantDependsOnVar(InvariantId dependent,
                                                     VarIdBase source) {
  assert(!dependent.equals(NULL_ID) && !source.equals(NULL_ID));
  if (_definingInvariant[source] == dependent) {
    logWarning("The dependent invariant ("
               << dependent << ") already "
               << "defines the source variable (" << source << "); "
               << "ignoring (selft-cyclic) dependency.");
    return;
  }
  _listeningInvariants[source].push_back(dependent);
  _inputVariables[dependent].push_back(source);
}

void PropagationGraph::registerDefinedVariable(VarIdBase dependent,
                                               InvariantId source) {
  assert(!dependent.equals(NULL_ID) && !source.equals(NULL_ID));
  if (_definingInvariant.at(dependent).id != NULL_ID.id) {
    throw VariableAlreadyDefinedException(
        "Variable " + std::to_string(dependent.id) +
        " already defined by invariant " +
        std::to_string(_definingInvariant.at(dependent).id));
  }
  Int index = -1;
  for (size_t i = 0; i < _listeningInvariants[dependent].size(); ++i) {
    if (_listeningInvariants[dependent][i] == source) {
      index = i;
      break;
    }
  }
  if (index >= 0) {
    _listeningInvariants[dependent].erase(
        _listeningInvariants[dependent].begin() + index);
    logWarning("The (self-cyclic) dependency that the source invariant "
               << "(" << source << ") depends on the dependent "
               << "variable (" << source << ") was removed.");
  }
  _definingInvariant[dependent] = source;
  _variablesDefinedByInvariant[source].push_back(dependent);
}

void PropagationGraph::close() {
  _isDecisionVar.resize(getNumVariables() + 1);
  _isOutputVar.resize(getNumVariables() + 1);
  for (size_t i = 1; i < getNumVariables() + 1; ++i) {
    _isOutputVar.at(i) = (_listeningInvariants.at(i).empty());
    _isDecisionVar.at(i) = (_definingInvariant.at(i) == NULL_ID);
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
