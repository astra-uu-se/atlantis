#include "cblsModel.hpp"

#include "core/propagationEngine.hpp"
#include "fznparser/modelfactory.hpp"
#include "invariantgraph/invariantGraphBuilder.hpp"
#include "search/neighbourhoods/maxViolating.hpp"
#include "search/searchProcedure.hpp"

static std::map<std::shared_ptr<fznparser::Variable>, VarId>
createLoggerVariableMap(
    const std::map<VarId, std::shared_ptr<fznparser::Variable>>&
        engineVariableMap) {
  std::map<std::shared_ptr<fznparser::Variable>, VarId> result;

  for (const auto& [varId, variable] : engineVariableMap) {
    if (!variable->annotations().has<fznparser::OutputAnnotation>()) {
      continue;
    }

    result.emplace(variable, varId);
  }

  return result;
}

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

  auto loggerVariableMap = createLoggerVariableMap(variableMap);
  MiniZincLogger logger(loggerVariableMap);
  search::MaxViolatingSearch searchProcedure(annealer, neighbourhood,
                                             assignment, logger);

  search::SearchContext context(100'000);
  searchProcedure.run(context);
}