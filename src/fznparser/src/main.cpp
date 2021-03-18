#include <iostream>

#include "FlatZincLexer.h"
#include "FlatZincParser.h"
#include "FznVisitor.h"
#include "statistics.hpp"
#include "structure_scheme_1.hpp"

using namespace fznparser;
using namespace antlr4;

int main(int argc, char* argv[]) {
  double tot1 = 0;
  double tot2 = 0;
  for (int i = 1; i < argc; i++) {
    try {
      std::ifstream stream;
      stream.open(argv[i]);
      ANTLRInputStream input(stream);

      FlatZincLexer lexer(&input);
      CommonTokenStream tokens(&lexer);

      FlatZincParser parser(&tokens);
      FlatZincParser::ModelContext* tree = parser.model();
      std::cerr << argv[i] << std::endl;

      FznVisitor visitor;
      Model m = visitor.visitModel(tree);
      Statistics s = Statistics(&m);

      m.init();

      StructureScheme1 ss1 = StructureScheme1(&m);

      ss1.scheme1();
      tot1 += s.score();
      std::cout << "Score1: " << s.score() << std::endl;

      ss1.clear();
      ss1.scheme2();
      std::cout << "Score2: " << s.score() << std::endl;
      tot2 += s.score();

    } catch (char const* msg) {
      std::cerr << msg << std::endl;
    }
  }
  tot1 = tot1 / argc;
  tot2 = tot2 / argc;
  std::cout << "Total Score1:" << tot1 << std::endl;
  std::cout << "Total Score2:" << tot2 << std::endl;

  return 0;
}
