#include <sequence/parallel_skip_list/include/augmented_skip_list.hpp>

#include <string>

#include <sequence/benchmarks/batch_sequence_benchmark/benchmark.hpp>
#include <utilities/include/utils.h>

namespace bsb = batch_sequence_benchmark;
using Element = parallel_skip_list::AugmentedElement;
using std::string;

int main(int argc, char** argv) {
  bsb::BenchmarkParameters parameters{bsb::GetBenchmarkParameters(argc, argv)};
  Element::Initialize();
  Element* elements{pbbs::new_array_no_init<Element>(parameters.num_elements)};
  pbbs::random r{};
  parallel_for (int i = 0; i < parameters.num_elements; i++) {
    new (&elements[i]) Element{r.ith_rand(i)};
  }

  bsb::RunBenchmark(elements, parameters);

  pbbs::delete_array(elements, parameters.num_elements);
  Element::Finish();
}
