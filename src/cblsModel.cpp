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
  invariantGraph->apply(engine);

  search::Annealer annealer;
  search::Assignment assignment(engine, invariantGraph->totalViolations()/*,
                                invariantGraph->objective()*/);

  // TODO: Why does getDecisionVariables return vector<VarIdBase> when all
  //  methods which query the engine take a VarId?
  std::vector<VarId> searchVariables;
  std::transform(engine.getDecisionVariables().begin(),
                 engine.getDecisionVariables().end(),
                 std::back_inserter(searchVariables),
                 [](auto varIdBase) { return VarId(varIdBase.id); });

  search::neighbourhoods::MaxViolatingNeighbourhood neighbourhood(
      searchVariables);
  search::MaxViolatingSearch searchProcedure(annealer, neighbourhood,
                                             assignment);

  search::SearchContext context(100'000);
  searchProcedure.run(context);
}