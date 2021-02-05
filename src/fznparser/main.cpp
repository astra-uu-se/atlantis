#include <iostream>
#include "FlatZincLexer.h"
#include "FlatZincParser.h"
#include "FznVisitor.h"

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

  std::cout << "Variables:" << std::endl;
  for (auto v : m._variables) {
    std::cout << v->_domain->getLb() << std::endl;
    std::cout << v->_domain->getUb() << std::endl;
  }

  return 0;
}
