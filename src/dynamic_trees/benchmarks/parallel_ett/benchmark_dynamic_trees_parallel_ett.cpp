#include <dynamic_trees/parallel_euler_tour_tree/include/euler_tour_tree.hpp>

#include <dynamic_trees/benchmarks/benchmark.hpp>

int main(int argc, char** argv) {
  dynamic_trees_benchmark::RunBenchmark<
      parallel_euler_tour_tree::EulerTourTree>(argc, argv);
  return 0;
}
