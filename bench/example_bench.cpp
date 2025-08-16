#include <benchmark/benchmark.h>
#include <vector>
#include <algorithm>

// Example benchmark: sorting a vector
static void BM_VectorSort(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    std::vector<int> data(state.range(0));
    std::generate(data.begin(), data.end(), std::rand);
    state.ResumeTiming();
    
    std::sort(data.begin(), data.end());
  }
  state.SetComplexityN(state.range(0));
}

BENCHMARK(BM_VectorSort)->Range(8, 8<<10)->Complexity();

// Example benchmark: simple function
static void BM_StringCreation(benchmark::State& state) {
  for (auto _ : state) {
    std::string empty_string;
  }
}

BENCHMARK(BM_StringCreation);

BENCHMARK_MAIN();
