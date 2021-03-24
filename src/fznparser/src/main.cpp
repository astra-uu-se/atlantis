#include <iostream>

#include "FlatZincLexer.h"
#include "FlatZincParser.h"
#include "FznVisitor.h"
#include "invariantstructure.hpp"

using namespace fznparser;
using namespace antlr4;

Model loadModel(std::string name);

int main(int argc, char* argv[]) {
  for (int i = 1; i < argc; i++) {
    try {
      InvariantStructure is = InvariantStructure(loadModel(argv[i]));
      is.run();
      // std::cout << "FILE: " << argv[i] << std::endl;
    } catch (char const* msg) {
      std::cerr << msg << std::endl;
    }
  }
  return 0;
}
Model loadModel(std::string name) {
  std::ifstream stream;
  stream.open(name);
  ANTLRInputStream input(stream);

  FlatZincLexer lexer(&input);
  CommonTokenStream tokens(&lexer);

  FlatZincParser parser(&tokens);
  FlatZincParser::ModelContext* tree = parser.model();

  FznVisitor visitor;
  return visitor.visitModel(tree);
}
