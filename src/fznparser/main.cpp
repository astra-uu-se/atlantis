#include <iostream>
#include "antlr4-runtime.h"
#include "FlatZincLexer.h"
#include "FlatZincParser.h"
#include "FznVisitor.h"

using namespace fznparser;
using namespace antlr4;

int main() {
  std::ifstream stream;
  stream.open("../fzn_examples/alldiff.fzn");
  ANTLRInputStream input(stream);

  FlatZincLexer lexer(&input);
  CommonTokenStream tokens(&lexer);

  FlatZincParser parser(&tokens);
  FlatZincParser::ModelContext* tree = parser.model();

  FznVisitor visitor;
  int x = visitor.visitModel(tree);
  std::cout << x << std::endl;

  return 0;
}
