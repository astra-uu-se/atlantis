#include "cblsModel.hpp"

#include "core/propagationEngine.hpp"
#include "fznparser/modelfactory.hpp"
#include "invariantgraph/invariantGraphBuilder.hpp"
#include "search/neighbourhoods/maxViolating.hpp"
#include "search/searchprocedure.hpp"

void CBLSModel::solve() {
  auto model = fznparser::ModelFactory::create(_modelPath);

  invariantgraph::InvariantGraphBuilder builder;
  auto invariantGraph = builder.build(model);

  PropagationEngine engine;
  auto variableMap = invariantGraph->apply(engine);

  search::Annealer annealer;
  search::Assignment assignment(engine, invariantGraph->totalViolations()/*,
                                invariantGraph->objective()*/);

  // TODO: Why does getDecisionVariables return vector<VarIdBase> when all
  //  methods which query the engine take a VarId?
  std::vector<VarId> searchVariables;
  for (auto varIdBase : engine.getDecisionVariables()) {
    VarId varId(varIdBase.id);

    if (engine.getLowerBound(varId) != engine.getUpperBound(varId)) {
      searchVariables.push_back(varId);
    }
  }
  
  search::neighbourhoods::MaxViolatingNeighbourhood neighbourhood(
      searchVariables);
  search::MaxViolatingSearch searchProcedure(annealer, neighbourhood,
                                             assignment);

  search::SearchContext context(100'000);
  searchProcedure.run(context, variableMap);
}