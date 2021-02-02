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
  Model m = visitor.visitModel(tree);

  std::cout << "Parameters:" << std::endl;
  for (auto p : m._parameters) {
   std::cout << p._name;
  }

  std::cout << "Variables:" << std::endl;
  for (auto v : m._variables) {
   std::cout << v._name << std::endl;
  }

  std::cout << "Constraints:" << std::endl;
  for (auto c : m._constraints) {
   std::cout << c._name << std::endl;
  }

  return 0;
}
