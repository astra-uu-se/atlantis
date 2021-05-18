#include <iostream>

#include "FlatZincLexer.h"
#include "FlatZincParser.h"
#include "FznVisitor.h"
#include "invariantstructure.hpp"

using namespace fznparser;
using namespace antlr4;

int main(int argc, char* argv[]) {
  int start = 1;
  std::string args;
  if (argc > 1 && argv[start][1] == '-') {
    start++;
  }
  if (argc > 1 && argv[start][0] == '-') {
    args = argv[start];
    start++;
  }
  if (argc < 1 + start) {
    std::cerr << "Not enough arguments." << std::endl;
    exit(1);
  }
  for (int i = start; i < argc; i++) {
    try {
      std::ifstream stream;
      stream.open(argv[i]);
      ANTLRInputStream input(stream);

      FlatZincLexer lexer(&input);
      CommonTokenStream tokens(&lexer);

      FlatZincParser parser(&tokens);
      FlatZincParser::ModelContext* tree = parser.model();

      FznVisitor visitor;
      Model m = visitor.visitModel(tree);
      InvariantStructure is = InvariantStructure(m, args);
      is.run();
      // std::cout << "FILE: " << argv[i] << std::endl;
      // is.line();
    } catch (char const* msg) {
      std::cerr << msg << std::endl;
    }
  }
  return 0;
}
