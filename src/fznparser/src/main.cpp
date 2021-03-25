#include <iostream>

#include "FlatZincLexer.h"
#include "FlatZincParser.h"
#include "FznVisitor.h"
#include "invariantstructure.hpp"

using namespace fznparser;
using namespace antlr4;

int main(int argc, char* argv[]) {
  for (int i = 1; i < 100; i++) {
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
      InvariantStructure is = InvariantStructure(m);
      is.run();
      std::cout << "FILE: " << argv[i] << std::endl;
    } catch (char const* msg) {
      std::cerr << msg << std::endl;
    }
  }
  return 0;
}
