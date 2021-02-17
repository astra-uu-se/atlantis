#include <iostream>

#include "FlatZincLexer.h"
#include "FlatZincParser.h"
#include "FznVisitor.h"
#include "model.hpp"

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
  m.init();

  if (m.hasCycle()) {
    std::cout << "FOUND CYCLE\n";
  } else {
    std::cout << "No cycle found\n";
  }

  return 0;
}
