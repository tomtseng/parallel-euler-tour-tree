#include <sequence/parallel_treap/include/treap.hpp>

#include <sequence/benchmarks/batch_sequence_benchmark/benchmark.hpp>
#include <utilities/include/random.h>
#include <utilities/include/utils.h>

namespace bsb = batch_sequence_benchmark;
using Element = treap::Node;

int main(int argc, char** argv) {
  bsb::BenchmarkParameters parameters{bsb::GetBenchmarkParameters(argc, argv)};
  Element* elements{pbbs::new_array_no_init<Element>(parameters.num_elements)};
  pbbs::random r{};
  parallel_for (int i = 0; i < parameters.num_elements; i++) {
    new (&elements[i]) Element{static_cast<unsigned>(r.ith_rand(i))};
  }

  bsb::RunBenchmark(elements, parameters);

  pbbs::delete_array(elements, parameters.num_elements);
}
