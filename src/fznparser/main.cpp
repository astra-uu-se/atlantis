#include <iostream>

#include "FlatZincLexer.h"
#include "FlatZincParser.h"
#include "FznVisitor.h"
#include "model.hpp"

using namespace fznparser;
using namespace antlr4;

int main() {
  std::ifstream stream;
  stream.open("../fzn_examples/input.fzn");
  ANTLRInputStream input(stream);

  FlatZincLexer lexer(&input);
  CommonTokenStream tokens(&lexer);

  FlatZincParser parser(&tokens);
  FlatZincParser::ModelContext* tree = parser.model();

  FznVisitor visitor;
  Model m = visitor.visitModel(tree);
  m.init();

  std::cout << "Constraint:" << std::endl;
  for (auto c : m._constraints) {
    c->print();
  }

  return 0;
}
