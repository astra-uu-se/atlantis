#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fznparser/modelFactory.hpp>
#include <iostream>

#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantGraphBuilder.hpp"
#include "invariantgraph/objectiveTranslator.hpp"
#include "search/assignment.hpp"
#include "search/neighbourhoods/randomNeighbourhood.hpp"
#include "search/searchProcedure.hpp"

TEST(FznNQueens, Solve) {
  std::filesystem::path modelFilePath("../fzn-models/n_queens.fzn");
  auto model = fznparser::ModelFactory::create(modelFilePath);

  // Not used for output, but this allows running end-to-end tests with the
  // CLI on the structure identification.
  invariantgraph::InvariantGraphBuilder invariantGraphBuilder;
  auto graph = invariantGraphBuilder.build(model);

  PropagationEngine engine;
  auto applicationResult = graph.apply(engine);

  search::Objective searchObjective(engine, model);
  engine.open();
  auto violation =
      searchObjective.registerWithEngine(applicationResult.totalViolations(),
                                         applicationResult.objectiveVariable());
  engine.close();

  search::Assignment assignment(engine, violation,
                                applicationResult.objectiveVariable(),
                                getObjectiveDirection(model.objective()));

  std::uint_fast32_t seed = std::time(nullptr);

  logDebug("Using seed " << seed);
  search::RandomProvider random(seed);

  auto neighbourhood = applicationResult.neighbourhood();
  search::SearchProcedure search(random, assignment, neighbourhood,
                                 searchObjective);

  search::SearchController::VariableMap flippedMap;
  for (const auto& [varId, fznVar] : applicationResult.variableMap())
    flippedMap.emplace(fznVar, varId);

  search::SearchController searchController = search::SearchController(
      model, flippedMap, std::chrono::milliseconds(1000));

  search::Annealer annealer(assignment, random);
  auto statistics = search.run(searchController, annealer);

  // Don't log to std::cout, since that would interfere with MiniZinc.
  statistics.display(std::cerr);
}