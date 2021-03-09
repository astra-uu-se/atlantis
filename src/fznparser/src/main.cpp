#include <iostream>

#include "FlatZincLexer.h"
#include "FlatZincParser.h"
#include "FznVisitor.h"
#include "statistics.hpp"
#include "structure_scheme_1.hpp"

using namespace fznparser;
using namespace antlr4;

int main(int argc, char* argv[]) {
  std::ifstream stream;
  stream.open(argv[1]);
  ANTLRInputStream input(stream);

  FlatZincLexer lexer(&input);
  CommonTokenStream tokens(&lexer);

  FlatZincParser parser(&tokens);
  FlatZincParser::ModelContext* tree = parser.model();

  FznVisitor visitor;
  Model m = visitor.visitModel(tree);
  Statistics s = Statistics(&m);

  m.init();

  StructureScheme1 ss1 = StructureScheme1(&m);

  ss1.findStructure();
  s.allStats(true);
  return 0;
}
