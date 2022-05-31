#include "search/neighbourhoods/allDifferentNeighbourhood.hpp"

#include "utils/flowNetwork.hpp"

static size_t findIndex(std::vector<Int>& values, Int value, size_t begin,
                        size_t end) {
  if (value == values[begin]) {
    return begin;
  }

  auto d = end - begin;
  if (d == 1) {
    return begin;
  }

  auto center = (begin + (d / 2));
  if (value < values[center]) {
    return findIndex(values, value, begin, center);
  }
  return findIndex(values, value, center, end);
}

static size_t findIndex(std::vector<Int>& values, Int value) {
  return findIndex(values, value, 0, values.size());
}

static utils::FlowNetwork makeFlowNetwork(
    std::vector<search::SearchVariable>& variables, std::vector<Int>& values) {
  // Source and sink as well.
  auto numNodes = variables.size() + values.size() + 2;
  auto source = numNodes - 2;
  auto sink = numNodes - 1;

  utils::FlowNetwork network(numNodes, source, sink);

  auto variableNodeIdx = [&](size_t variableIdx) -> size_t {
    return variableIdx;
  };

  auto valueNodeIdx = [&](Int value) -> size_t {
    auto idx = findIndex(values, value);
    return variables.size() + idx;
  };

  for (auto i = 0u; i < variables.size(); i++) {
    network.addEdge(source, variableNodeIdx(i), 1);

    for (auto value : variables[i].domain().values()) {
      auto valueNode = valueNodeIdx(value);
      network.addEdge(variableNodeIdx(i), valueNode, 1);
      network.addEdge(valueNode, sink, 1);
    }
  }

  return network;
}

search::neighbourhoods::AllDifferentNeighbourhood::AllDifferentNeighbourhood(
    std::vector<search::SearchVariable> variables)
    : _variables(std::move(variables)) {
  assert(_variables.size() > 1);

  std::unordered_set<Int> values;
  _minVal = std::numeric_limits<Int>::max();
  _maxVal = std::numeric_limits<Int>::min();
  for (auto& variable : _variables) {
    _minVal = std::min(_minVal, variable.domain().lowerBound());
    _maxVal = std::max(_maxVal, variable.domain().upperBound());

    auto vals = variable.domain().values();
    values.insert(vals.begin(), vals.end());
  }

  _values = std::vector<Int>(values.begin(), values.end());
  std::sort(_values.begin(), _values.end());

  _flowNetwork = makeFlowNetwork(_variables, _values);
}

void search::neighbourhoods::AllDifferentNeighbourhood::initialise(
    RandomProvider& random, AssignmentModifier& modifications) {
  _freeValues.clear();
  for (Int i = _minVal; i <= _maxVal; ++i) {
    _freeValues.emplace(i);
  }

  auto matchingSize = createMatching(random, modifications);
  if (matchingSize != _variables.size()) {
    throw std::runtime_error(
        "Failed to create assignment satisfying all different.");
  }
}

bool search::neighbourhoods::AllDifferentNeighbourhood::randomMove(
    RandomProvider& random, Assignment& assignment, Annealer& annealer) {
  // This is a mega shortcut, and depends on domain constraints being posted
  // for the variables covered by this domain. This neighbourhood will assign
  // values to variables outside their domains if the domains of the variables
  // are different.
  // TODO: Make the implementation 100% correct, even for variables with
  //       different domains.

  if (_freeValues.empty() || random.boolean()) {
    return swapValues(random, assignment, annealer);
  }

  return assignValue(random, assignment, annealer);
}

bool search::neighbourhoods::AllDifferentNeighbourhood::swapValues(
    search::RandomProvider& random, search::Assignment& assignment,
    search::Annealer& annealer) {
  size_t i = random.intInRange(0, static_cast<Int>(_variables.size()) - 1);
  size_t j =
      (i + random.intInRange(1, static_cast<Int>(_variables.size()) - 1)) %
      _variables.size();

  auto var1 = _variables[i].engineId();
  auto var2 = _variables[j].engineId();

  Int value1 = assignment.value(var1);
  Int value2 = assignment.value(var2);

  return maybeCommit(Move<2>({var1, var2}, {value2, value1}), assignment,
                     annealer);
}

bool search::neighbourhoods::AllDifferentNeighbourhood::assignValue(
    search::RandomProvider& random, search::Assignment& assignment,
    search::Annealer& annealer) {
  auto variable = random.element(_variables);
  auto oldValue = assignment.value(variable.engineId());
  auto newValue = random.inDomain(variable.domain());

  if (annealer.acceptMove(Move<1>({variable.engineId()}, {newValue}))) {
    _freeValues.emplace(oldValue);
    _freeValues.erase(newValue);

    return true;
  }

  return false;
}

static Int bfs(utils::FlowNetwork& graph, std::vector<int>& parent) {
  std::fill(parent.begin(), parent.end(), -1);
  parent[graph.source()] = -2;
  std::queue<std::pair<size_t, size_t>> q;
  q.push({graph.source(), std::numeric_limits<size_t>::max()});

  while (!q.empty()) {
    size_t cur = q.front().first;
    size_t flow = q.front().second;
    q.pop();

    for (auto next : graph.edges(cur)) {
      if (parent[next] == -1 && graph.remainingCapacity(cur, next) > 0) {
        parent[next] = cur;
        int new_flow = std::min(flow, graph.capacity(cur, next));
        if (next == graph.sink()) return new_flow;
        q.push({next, new_flow});
      }
    }
  }

  return 0;
}

static Int edmundsKarp(utils::FlowNetwork& graph) {
  Int flow = 0;
  std::vector<int> parent(graph.size());
  Int new_flow;

  while ((new_flow = bfs(graph, parent)) != 0) {
    flow += new_flow;
    auto cur = graph.sink();
    while (cur != graph.source()) {
      int prev = parent[cur];
      graph.remainingCapacity(prev, cur) -= new_flow;
      graph.remainingCapacity(cur, prev) += new_flow;
      cur = prev;
    }
  }

  return flow;
}

size_t search::neighbourhoods::AllDifferentNeighbourhood::createMatching(
    RandomProvider& random, AssignmentModifier& modifications) {
  _flowNetwork.resetFlows();
  _flowNetwork.shuffleAdjacencyList(random.generator());
  auto size = edmundsKarp(_flowNetwork);

  for (auto i = 0u; i < _variables.size(); i++) {
    for (auto& to : _flowNetwork.edges(i)) {
      if (_flowNetwork.remainingCapacity(i, to) == 0) {
        // There is only one edge with a flow outgoing from _variables[i].
        auto value = _values[to - _variables.size()];
        _freeValues.erase(value);
        modifications.set(_variables[i].engineId(), value);
        break;
      }
    }
  }

  return size;
}
