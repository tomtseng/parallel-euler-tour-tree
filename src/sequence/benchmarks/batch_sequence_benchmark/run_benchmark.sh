#!/bin/bash -x

num_elements=100000000
batch_size=10000000
batch_sizes=(100 1000 10000 100000 1000000 10000000 99999999)
threads=(1 2 4 8 16 32 64 72 144)
iters=3

bin_dir=$(git rev-parse --show-toplevel)/bin
time_dir=times
threads_times_output=${time_dir}/threads.txt
batch_times_output=${time_dir}/batch-shuffle.txt
batch_backward_times_output=${time_dir}/batch-backward.txt

mkdir $time_dir
rm -i $threads_times_output $batch_times_output $batch_backward_times_output

echo "### n=${num_elements} k=${batch_size} iters=${iters}" >> $threads_times_output
echo "### n=${num_elements} iters=${iters}" | tee -a $batch_times_output $batch_backward_times_output

cd parallel_augmented_skip_list
make -s
benchmark_bin=${bin_dir}/benchmark_batch_sequence_parallel_augmented_skip_list
for t in ${threads[@]}
do
  CILK_NWORKERS=$t numactl -i all $benchmark_bin -n $num_elements -k $batch_size -iters $iters >> ../${threads_times_output}
done
echo "** parallel_augmented_skip_list (1) ************************" | tee -a ../$batch_times_output ../$batch_backward_times_output
for k in ${batch_sizes[@]}
do
  CILK_NWORKERS=1   $benchmark_bin -n $num_elements -k $k -iters $iters                      >> ../$batch_times_output
  CILK_NWORKERS=1   $benchmark_bin -n $num_elements -k $k -iters $iters -batch-type backward >> ../$batch_backward_times_output
done
echo "** parallel_augmented_skip_list (72h) ************************" | tee -a ../$batch_times_output ../$batch_backward_times_output
for k in ${batch_sizes[@]}
do
  CILK_NWORKERS=144 numactl -i all $benchmark_bin -n $num_elements -k $k -iters $iters                      >> ../$batch_times_output
  CILK_NWORKERS=144 numactl -i all $benchmark_bin -n $num_elements -k $k -iters $iters -batch-type backward >> ../$batch_backward_times_output
done
cd ..

cd augmented_skip_list
make -s
benchmark_bin=${bin_dir}/benchmark_batch_sequence_augmented_skip_list
echo "** augmented_skip_list ************************" | tee -a ../$batch_times_output ../$batch_backward_times_output
for k in ${batch_sizes[@]}
do
  $benchmark_bin -n $num_elements -k $k -iters $iters                      >> ../$batch_times_output
  $benchmark_bin -n $num_elements -k $k -iters $iters -batch-type backward >> ../$batch_backward_times_output
done
cd ..

cd parallel_treap
make -s
benchmark_bin=${bin_dir}/benchmark_batch_sequence_parallel_treap
echo "** parallel_treap (72h) ************************" | tee -a ../$batch_times_output ../$batch_backward_times_output
for k in ${batch_sizes[@]}
do
  CILK_NWORKERS=144 numactl -i all $benchmark_bin -n $num_elements -k $k -iters $iters                      >> ../$batch_times_output
done
cd ..
