This benchmark measures the performance of splits and joins on sequence data
structures.

This was written to run some one-off experiments, so it was not properly
designed to be easy to use, easy to modify, or easy to interpret.

### How do I run it?

Primarily this is run through `run_benchmark.sh`, perhaps after fiddling with
some of the constants at the top of the script file. This benchmarks some
sequence implementations with varying numbers of threads and varying batch
sizes. Timings are output to the `times/` directory.

Another way is to go into one of the implementation directories, `make` the
benchmark, and run
```
<base code directory>/bin/benchmark_sequence_<implementation> -n <sequence_length> -k <batch_size> -iters <number of iterations>
```

### What does it time?

Fix a batch of random indices. For some number of iterations, construct a linear
sequence of a fixed length, split on all the indices, and join on all the
indices. Output the median time to perform the splits and the joins.

The splits in a trial are run all at once with a `parallel_for`, and the same is
true for the joins. As such, tested implementations must either be
phase-concurrent or be compiled to be sequential.
