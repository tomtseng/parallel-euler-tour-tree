#include <sequence/splay_tree/include/splay_tree.hpp>

#include <sequence/benchmarks/sequence_benchmark/benchmark.hpp>
#include <utilities/include/utils.h>

namespace sb = sequence_benchmark;
using Node = splay_tree::Node;

int main(int argc, char** argv) {
  sb::BenchmarkParameters parameters{sb::GetBenchmarkParameters(argc, argv)};
  Node* nodes{pbbs::new_array<Node>(parameters.num_elements)};

  sequence_benchmark::RunBenchmark(nodes, parameters);

  pbbs::delete_array(nodes, parameters.num_elements);
}
