#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <constraints/allDifferent.hpp>
#include <core/propagationEngine.hpp>
#include <invariants/linear.hpp>
#include <iostream>
#include <numeric>
#include <random>
#include <tuple>
#include <utility>
#include <vector>

const int NUM_SUDOKUS = 1;
const size_t SIZE = 9;
const size_t SQRT_SIZE = 3;

/**
 * The sudoku's to run the benchmarks on. Below are the sources.
 *
 * 1. https://en.wikipedia.org/wiki/Sudoku
 */
const int SUDOKUS[NUM_SUDOKUS][SIZE][SIZE] = {{{5, 3, 0, 0, 7, 0, 0, 0, 0},
                                               {6, 0, 0, 1, 9, 5, 0, 0, 0},
                                               {0, 9, 8, 0, 0, 0, 0, 6, 0},
                                               {8, 0, 0, 0, 6, 0, 0, 0, 3},
                                               {4, 0, 0, 8, 0, 3, 0, 0, 1},
                                               {7, 0, 0, 0, 2, 0, 0, 0, 6},
                                               {0, 6, 0, 0, 0, 0, 2, 8, 0},
                                               {0, 0, 0, 4, 1, 9, 0, 0, 5},
                                               {0, 0, 0, 0, 8, 0, 0, 7, 9}}};

class Sudoku : public benchmark::Fixture {
 public:
  std::unique_ptr<PropagationEngine> engine;

  std::vector<std::vector<VarId>> sudokuRows;  // The rows of the sudoku.
  std::vector<std::vector<VarId>> sudokuCols;  // The columns of the sudoku.

  std::vector<std::tuple<int, int>> freeCells;  // Which row,col cells are free.

  std::vector<VarId> allDiffViolations;
  VarId totalViolations;

  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<> distr;

  void SetUp(const ::benchmark::State& state) {
    engine = std::make_unique<PropagationEngine>();
    gen = std::mt19937(rd());

    engine->open();

    switch (state.range(0)) {
      case 0:
        engine->setPropagationMode(
            PropagationEngine::PropagationMode::INPUT_TO_OUTPUT);
        break;
      case 1:
        engine->setPropagationMode(PropagationEngine::PropagationMode::MIXED);
        break;
      case 2:
        engine->setPropagationMode(
            PropagationEngine::PropagationMode::OUTPUT_TO_INPUT);
        break;
    }

    int sudokuId = state.range(1);
    sudokuRows.resize(SIZE);
    sudokuCols.resize(SIZE);
    freeCells.reserve(SIZE * SIZE);
    allDiffViolations.reserve(2 * SIZE);

    for (int i = 0; i < static_cast<int>(SIZE); i++) {
      sudokuCols[i].resize(SIZE);
      sudokuRows[i].resize(SIZE);
    }

    // Setup the variables for the cells of the sudoku.
    for (int row = 0; row < static_cast<int>(SIZE); row++) {
      for (int col = 0; col < static_cast<int>(SIZE); col++) {
        if (SUDOKUS[sudokuId][row][col] == 0) {
          freeCells.push_back(std::make_tuple(row, col));
        }

        Int upperbound = SUDOKUS[sudokuId][row][col] == 0
                             ? SIZE
                             : SUDOKUS[sudokuId][row][col];

        Int lowerbound =
            SUDOKUS[sudokuId][row][col] == 0 ? 1 : SUDOKUS[sudokuId][row][col];

        int initialValue = 1;  // We don't care about solving for now

        auto cellVariable =
            engine->makeIntVar(initialValue, lowerbound, upperbound);

        sudokuRows[row][col] = cellVariable;
        sudokuCols[col][row] = cellVariable;
      }
    }

    // Setup the violation variables.
    for (int i = 0; i < static_cast<int>(SIZE); i++) {
      VarId rowViolation = engine->makeIntVar(0, 0, SIZE);
      VarId colViolation = engine->makeIntVar(0, 0, SIZE);
      allDiffViolations.push_back(rowViolation);
      allDiffViolations.push_back(colViolation);

      engine->makeConstraint<AllDifferent>(rowViolation, sudokuRows[i]);
      engine->makeConstraint<AllDifferent>(colViolation, sudokuCols[i]);
    }

    totalViolations = engine->makeIntVar(0, 0, SIZE * SIZE);
    engine->makeInvariant<Linear>(allDiffViolations, totalViolations);

    engine->close();

    distr =
        std::uniform_int_distribution<>{0, static_cast<int>(freeCells.size())};
  }

  void TearDown(const ::benchmark::State&) {
    allDiffViolations.clear();
    sudokuRows.clear();
    sudokuCols.clear();
    freeCells.clear();
  }
};

BENCHMARK_DEFINE_F(Sudoku, probing_single_swap)(benchmark::State& st) {
  for (auto _ : st) {
    std::tuple<int, int> iPos = freeCells[distr(gen)];
    std::tuple<int, int> jPos = freeCells[distr(gen)];

    VarId i = sudokuRows[std::get<0>(iPos)][std::get<1>(iPos)];
    VarId j = sudokuRows[std::get<0>(jPos)][std::get<1>(jPos)];

    Int oldI = engine->getCommittedValue(i);
    Int oldJ = engine->getCommittedValue(j);

    // Perform random swap
    engine->beginMove();
    engine->setValue(i, oldJ);
    engine->setValue(j, oldI);
    engine->endMove();

    engine->beginQuery();
    engine->query(totalViolations);
    engine->endQuery();
  }
}

static void arguments(benchmark::internal::Benchmark* benchmark) {
  for (int sudokuId = 0; sudokuId < NUM_SUDOKUS; sudokuId++) {
    for (int mode = 0; mode <= 2; ++mode) {
      benchmark->Args({mode, sudokuId});
    }
  }
}

BENCHMARK_REGISTER_F(Sudoku, probing_single_swap)
    ->Unit(benchmark::kMillisecond)
    ->Apply(arguments);