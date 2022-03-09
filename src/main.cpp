#include <chrono>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>

#include "core/propagationEngine.hpp"
#include "fznparser/modelfactory.hpp"
#include "invariantgraph/invariantGraphBuilder.hpp"

/**
 * @brief Read a duration in milliseconds from an input stream. Used to allow
 * cxxopts to parse the duration for us.
 *
 * @param is The input stream to parse from.
 * @param duration The reference to the value that holds the duration.
 * @return std::istream& The modified input stream.
 */
std::istream& operator>>(std::istream& is, std::chrono::milliseconds& duration);

int main(int argc, char* argv[]) {
  try {
    cxxopts::Options options(
        std::string(argv[0]),
        "Constraint-based local search backend for MiniZinc.");

    options.positional_help("[flatzinc file]").show_positional_help();

    // Clang Format will make these option definitions completely garbled, hence
    // the comments to turn it off for this region.
    // clang-format off
    options.add_options()
      (
        "n,num-solutions",
        "Instructs the solver to stop after reporting i solutions (only used with satisfaction problems).",
        cxxopts::value<uint>()->default_value("1")
      )
      (
        "t,time-limit",
        "Wall time limit in milliseconds.",
        cxxopts::value<std::chrono::milliseconds>()->default_value("1")
      )
      ("help", "Print help");

    options.add_options("Positional")
      (
        "modelFile",
        "Path to the flatzinc file which contains the model.",
        cxxopts::value<std::filesystem::path>()
      );
    // clang-format on

    options.parse_positional({"modelFile"});

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help({""}) << std::endl;
      return 0;
    }

    auto& modelFilePath = result["modelFile"].as<std::filesystem::path>();
    auto model = fznparser::ModelFactory::create(modelFilePath);
    std::cout << "Model has " << model->variables().size() << " variables and "
              << model->constraints().size() << " constraints." << std::endl;

    // Not used for output, but this allows running end-to-end tests with the
    // CLI on the structure identification.
    invariantgraph::InvariantGraphBuilder invariantGraphBuilder;
    auto graph = invariantGraphBuilder.build(model);

    PropagationEngine engine;
    graph->apply(engine);

    auto numDecisionVars = engine.getDecisionVariables().size();
    auto numVars = engine.getNumVariables();
    std::cout << "Converted to " << numDecisionVars
              << " decision variables and " << (numVars - numDecisionVars)
              << " defined variables." << std::endl;
  } catch (const cxxopts::OptionException& e) {
    std::cerr << "error: " << e.what() << std::endl;
  } catch (const std::invalid_argument& e) {
    std::cerr << "error: " << e.what() << std::endl;
  }
}

std::istream& operator>>(std::istream& is,
                         std::chrono::milliseconds& duration) {
  long x;
  auto& is2 = is >> x;

  duration = std::chrono::milliseconds(x);

  return is2;
}