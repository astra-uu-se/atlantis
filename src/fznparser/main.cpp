#include <iostream>
#include "antlr4-runtime.h"
#include "FlatZincLexer.h"
#include "FlatZincParser.h"

using namespace fznparser;
using namespace antlr4;

int main() {
  std::ifstream stream;
  stream.open("../fzn_examples/input.fzn");
  // ANTLRInputStream input("var int: x; solve satisfy;");
  ANTLRInputStream input(stream);

  FlatZincLexer lexer(&input);
  CommonTokenStream tokens(&lexer);

  tokens.fill();
  for (auto token : tokens.getTokens()) {
    std::cout << token->toString() << std::endl;
  }

  FlatZincParser parser(&tokens);
  tree::ParseTree* tree = parser.model();

  std::cout << tree->toStringTree(&parser) << std::endl << std::endl;
  return 0;
}
