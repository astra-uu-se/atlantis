#include <iostream>

#include "FlatZincLexer.h"
#include "FlatZincParser.h"
#include "FznVisitor.h"
#include "statistics.hpp"

using namespace fznparser;
using namespace antlr4;

int main() {
  std::ifstream stream;
  stream.open("fzn_examples/alldiff.fzn");
  ANTLRInputStream input(stream);

  FlatZincLexer lexer(&input);
  CommonTokenStream tokens(&lexer);

  FlatZincParser parser(&tokens);
  FlatZincParser::ModelContext* tree = parser.model();

  FznVisitor visitor;
  Model m = visitor.visitModel(tree);
  Statistics s = Statistics(&m);
  m.init();
  m.findStructure();
  s.variablesDefinedBy();
  s.countDefinedVariables();
  return 0;
}
