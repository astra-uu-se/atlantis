#include "propagation/bottomUpPropagationGraph.hpp"

BottomUpPropagationGraph::BottomUpPropagationGraph(size_t expectedSize)
    : PropagationGraph(expectedSize){
}

void BottomUpPropagationGraph::notifyMaybeChanged(const Timestamp& t, VarId id) {

}

// VarId BottomUpPropagationGraph::getNextStableVariable(const Timestamp& t) {
// }

void BottomUpPropagationGraph::registerVar(VarId id) {
  PropagationGraph::registerVar(id);  // call parent implementation
}