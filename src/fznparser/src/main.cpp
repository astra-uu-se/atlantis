#include <iostream>

#include "FlatZincLexer.h"
#include "FlatZincParser.h"
#include "FznVisitor.h"
#include "statistics.hpp"
#include "structure_scheme_1.hpp"

using namespace fznparser;
using namespace antlr4;

int main(int argc, char* argv[]) {
  int count = 0;
  int n = 6;
  double tot[n];
  for (int i = 0; i < n; i++) {
    tot[i] = 0;
  }
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

      ss1.scheme(0);
      if (s.score() <= 0) continue;
      count++;
      for (int j = 0; j < n; j++) {
        m = visitor.visitModel(tree);
        m.init();
        s = Statistics(&m);
        ss1 = StructureScheme1(&m);
        ss1.scheme(j);
        std::cout << "Score" << j + 1 << ": " << s.score() << std::endl;
        tot[j] += s.score();
      }

    } catch (char const* msg) {
      std::cerr << msg << std::endl;
    }
  }
  for (int j = 0; j < n; j++) {
    tot[j] = tot[j] / count;
    std::cout << "Total Score" << j + 1 << ": " << tot[j] << std::endl;
  }
  return 0;
}
