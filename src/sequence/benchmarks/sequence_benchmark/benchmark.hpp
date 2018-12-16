#pragma once

#include <cassert>
#include <iostream>
#include <random>
#include <vector>

#include <utilities/include/debug.hpp>
#include <utilities/include/gettime.h>
#include <utilities/include/parse_command_line.h>
#include <utilities/include/utils.h>

namespace sequence_benchmark {

struct BenchmarkParameters {
  int num_elements;
  int batch_size;
  int num_iterations;
};

BenchmarkParameters GetBenchmarkParameters(int argc, char** argv) {
  commandLine P{argc, argv,
      "./benchmark [-n (num elements)] [-k (batch size)] [-iters]"};
  BenchmarkParameters parameters{};
  parameters.num_elements = P.getOptionIntValue("-n", 1000);
  parameters.batch_size =
      P.getOptionIntValue("-k", parameters.num_elements / 32);
  parameters.num_iterations = P.getOptionIntValue("-iters", 5);
  return parameters;
}

// Pick `batch_size` many element locations at random. For `num_iterations`
// iterations, construct a list and then split and join on those locations.
// Report the median time to perform all these splits and joins.
template <typename Element>
void RunBenchmark(Element* elements, const BenchmarkParameters& parameters) {
  const int num_elements{parameters.num_elements};
  const int batch_size{parameters.batch_size};
  const int num_iterations{parameters.num_iterations};

  assert(0 <= batch_size && batch_size < num_elements);
  std::cout << "Running with " << nworkers() << " workers" << std::endl;

  int* perm{pbbs::new_array_no_init<int>(num_elements - 1)};
  for (int i = 0; i < num_elements - 1; i++) {
    perm[i] = i;
  }
  std::mt19937 generator{0};
  std::shuffle(perm, perm + num_elements - 1, generator);

  std::vector<double> split_times(num_iterations);
  std::vector<double> join_times(num_iterations);

  for (int j = 0; j < num_iterations; j++) {
    // construct list
    parallel_for (int i = 0; i < num_elements - 1; i++) {
      Element::Join(&elements[perm[i]], &elements[perm[i] + 1]);
    }

    timer split_t; split_t.start();
    parallel_for (int i = 0; i < batch_size; i++) {
      elements[perm[i]].Split();
    }
    split_times[j] = split_t.stop();

    timer join_t; join_t.start();
    parallel_for (int i = 0; i < batch_size; i++) {
      int idx{perm[i]};
      Element::Join(&elements[idx], &elements[idx + 1]);
    }
    join_times[j] = join_t.stop();

    // destroy list
    parallel_for (int i = 0; i < num_elements - 1; i++) {
      elements[i].Split();
    }
  }

  timer::report_time_no_newline("join", median(join_times));
  timer::report_time("split", median(split_times));

  // Element* representative_0{elements[0].FindRepresentative()};
  // parallel_for (int i = 0; i < num_elements; i++) {
  //   assert(representative_0 == elements[i].FindRepresentative());
  // }

  pbbs::delete_array(perm, num_elements - 1);
}

}  // namespace sequence_benchmark
