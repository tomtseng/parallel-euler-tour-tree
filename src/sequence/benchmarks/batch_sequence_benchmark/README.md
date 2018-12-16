This benchmark measures the performance of batch splits and batch joins on
sequence data structures.

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
<base code directory>/bin/benchmark_batch_sequence_<implementation> -n <sequence_length> -k <batch_size> -iters <number of iterations> (-batch-type <random or backward>)
```

### What does it time?

Fix a batch of indices. For some number of iterations, construct a linear
sequence of a fixed length, batch split on all the indices, and batch join on
all the indices. Output the median time to perform the batch split and the batch
join.
