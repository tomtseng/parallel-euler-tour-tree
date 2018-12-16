#include <dynamic_trees/link_cut_tree/include/link_cut_tree.hpp>

#include <dynamic_trees/benchmarks/benchmark.hpp>

int main(int argc, char** argv) {
  dynamic_trees_benchmark::RunBenchmark<link_cut_tree::LinkCutTree>(argc, argv);
  return 0;
}
