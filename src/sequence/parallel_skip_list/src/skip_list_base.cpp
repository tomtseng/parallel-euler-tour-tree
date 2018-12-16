#include <sequence/parallel_skip_list/include/skip_list_base.hpp>

namespace parallel_skip_list {

namespace _internal {

constexpr int kMaxHeight{concurrent_array_allocator::kMaxArrayLength};

int GenerateHeight(size_t random_int) {
  int h{1};
  // Geometric(1/2) distribution.
  // Note: could also consider Geometric(3/4) (so 1/4 of elements go up to next
  // level) by reading two bits at once
  while (random_int & 1) {
    random_int >>= 1;
    h++;
  }
  return min(h, kMaxHeight);
}

}  // namespace _internal

}  // namespace parallel_skip_list
