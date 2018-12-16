#include <sequence/skip_list/include/skip_list.hpp>

#include <sequence/benchmarks/sequence_benchmark/benchmark.hpp>
#include <utilities/include/utils.h>

namespace sb = sequence_benchmark;
using Element = skip_list::Element;

int main(int argc, char** argv) {
  sb::BenchmarkParameters parameters{sb::GetBenchmarkParameters(argc, argv)};
  Element* elements{pbbs::new_array_no_init<Element>(parameters.num_elements)};
  pbbs::random r{};
  for (int i = 0; i < parameters.num_elements; i++) {
    new (&elements[i]) Element{r.ith_rand(i)};
  }

  sb::RunBenchmark(elements, parameters);

  pbbs::delete_array(elements, parameters.num_elements);
}
