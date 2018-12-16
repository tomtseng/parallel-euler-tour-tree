#include <dynamic_trees/euler_tour_tree/include/skip_list_ett.hpp>

#include <dynamic_trees/benchmarks/benchmark.hpp>

int main(int argc, char** argv) {
  dynamic_trees_benchmark::RunBenchmark<skip_list_ett::EulerTourTree>(
        argc, argv);
  return 0;
}
