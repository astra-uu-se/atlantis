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
  double tot1 = 0;
  double tot2 = 0;
  double tot3 = 0;
  double tot4 = 0;
  double tot5 = 0;
  double tot6 = 0;
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
      if (s.score() <= 0) continue;
      count++;
      tot1 += s.score();
      std::cout << "Score1: " << s.score() << std::endl;

      m = visitor.visitModel(tree);
      m.init();
      ss1 = StructureScheme1(&m);

      ss1.scheme2();
      std::cout << "Score2: " << s.score() << std::endl;
      tot2 += s.score();

      m = visitor.visitModel(tree);
      m.init();
      ss1 = StructureScheme1(&m);

      ss1.scheme3();
      std::cout << "Score3: " << s.score() << std::endl;
      tot3 += s.score();

      m = visitor.visitModel(tree);
      m.init();
      ss1 = StructureScheme1(&m);

      ss1.scheme4();
      std::cout << "Score4: " << s.score() << std::endl;
      tot4 += s.score();

      m = visitor.visitModel(tree);
      m.init();
      ss1 = StructureScheme1(&m);

      ss1.scheme5();
      std::cout << "Score5: " << s.score() << std::endl;
      tot5 += s.score();

      m = visitor.visitModel(tree);
      m.init();
      ss1 = StructureScheme1(&m);

      ss1.scheme6();
      std::cout << "Score6: " << s.score() << std::endl;
      tot6 += s.score();

    } catch (char const* msg) {
      std::cerr << msg << std::endl;
    }
  }
  tot1 = tot1 / count;
  tot2 = tot2 / count;
  tot3 = tot3 / count;
  tot4 = tot4 / count;
  tot5 = tot5 / count;
  tot6 = tot6 / count;
  std::cout << "Total Score1: " << tot1 << std::endl;
  std::cout << "Total Score2: " << tot2 << std::endl;
  std::cout << "Total Score3: " << tot3 << std::endl;
  std::cout << "Total Score4: " << tot4 << std::endl;
  std::cout << "Total Score5: " << tot5 << std::endl;
  std::cout << "Total Score6: " << tot6 << std::endl;

  return 0;
}
