#include <dynamic_trees/euler_tour_tree/include/splay_tree_ett.hpp>

#include <dynamic_trees/benchmarks/benchmark.hpp>

int main(int argc, char** argv) {
  dynamic_trees_benchmark::RunBenchmark<splay_tree_ett::EulerTourTree>(
      argc, argv);
  return 0;
}
