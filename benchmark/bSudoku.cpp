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

  // Which row,col cells are free, indexed by the block they belong to.
  std::array<std::vector<std::tuple<int, int>>, SIZE> freeCells;
  // What values are missing in each block. These are the values to pick from
  // when assigning values to variables.
  std::array<std::set<Int>, SIZE> missingValues;
  // Contains the possible values to pick from for each block. It differs from
  // missingValues because this will be mutated during the construction of the
  // initial assignment to ensure no duplicate values are assigned to the same
  // block.
  std::array<std::set<Int>, SIZE> possibleValues;

  std::vector<VarId> allDiffViolations;
  VarId totalViolations;

  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<> blockDistr;

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
    allDiffViolations.reserve(2 * SIZE);

    for (int i = 0; i < static_cast<int>(SIZE); i++) {
      sudokuCols[i].resize(SIZE);
      sudokuRows[i].resize(SIZE);
    }

    computeMissingValues(sudokuId);
    std::copy(missingValues.begin(), missingValues.end(),
              possibleValues.begin());

    // Setup the variables for the cells of the sudoku.
    for (int row = 0; row < static_cast<int>(SIZE); row++) {
      for (int col = 0; col < static_cast<int>(SIZE); col++) {
        int block = (row - (row % SQRT_SIZE)) + col / SQRT_SIZE;

        if (SUDOKUS[sudokuId][row][col] == 0) {
          freeCells[block].push_back(std::make_tuple(row, col));
        }

        Int initialValue = SUDOKUS[sudokuId][row][col];
        if (initialValue == 0) {
          initialValue = pullAvailableValue(block);
        }

        assert(1 <= initialValue && initialValue <= static_cast<Int>(SIZE));

        auto cellVariable = engine->makeIntVar(initialValue, 1, SIZE);

        sudokuRows[row][col] = cellVariable;
        sudokuCols[col][row] = cellVariable;
      }
    }

#ifndef NDEBUG
    for (size_t block = 0; block < SIZE; block++) {
      assert(possibleValues[block].empty());
    }
#endif

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

    blockDistr = std::uniform_int_distribution<>{0, static_cast<int>(SIZE) - 1};
  }

  void TearDown(const ::benchmark::State&) {
    allDiffViolations.clear();
    sudokuRows.clear();
    sudokuCols.clear();

    std::for_each(freeCells.begin(), freeCells.end(),
                  [](auto& vec) { vec.clear(); });
  }

 private:
  Int pullAvailableValue(int blockId) {
    assert(!possibleValues[blockId].empty());

    size_t originalSize = possibleValues[blockId].size();
    std::uniform_int_distribution idxDistr(
        0, static_cast<int>(possibleValues[blockId].size()) - 1);

    std::set<Int>::const_iterator it = possibleValues[blockId].cbegin();
    int offset = idxDistr(gen);
    std::advance(it, offset);

    Int value = *it;
    possibleValues[blockId].erase(value);

    size_t newSize = possibleValues[blockId].size();
    assert(originalSize - newSize == 1);

    return value;
  }

  void computeMissingValues(int sudokuId) {
    for (size_t block = 0; block < SIZE; block++) {
      missingValues[block] = std::set<Int>();
      for (Int value = 1; value <= static_cast<Int>(SIZE); value++) {
        missingValues[block].insert(value);
      }
    }

    for (size_t block = 0; block < SIZE; block++) {
      for (size_t row = 0; row < SQRT_SIZE; row++) {
        for (size_t col = 0; col < SQRT_SIZE; col++) {
          int global_row = (block / SQRT_SIZE) * SQRT_SIZE + row;
          int global_col = (block % SQRT_SIZE) * SQRT_SIZE + col;
          Int value = SUDOKUS[sudokuId][global_row][global_col];

          if (value > 0) {
            missingValues[block].erase(value);
          }
        }
      }
    }
  }
};

BENCHMARK_DEFINE_F(Sudoku, probing_single_swap)(benchmark::State& st) {
  for (auto _ : st) {
    size_t block = static_cast<size_t>(blockDistr(gen));

    std::uniform_int_distribution idxDistr(
        0, static_cast<int>(freeCells[block].size()) - 1);

    std::tuple<int, int> iPos = freeCells[block][idxDistr(gen)];
    std::tuple<int, int> jPos = freeCells[block][idxDistr(gen)];

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