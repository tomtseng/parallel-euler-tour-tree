#include <sequence/parallel_skip_list/include/skip_list.hpp>

#include <cassert>

#include <utilities/include/debug.hpp>
#include <utilities/include/random.h>
#include <utilities/include/utils.h>

using Element = parallel_skip_list::Element;

constexpr int kNumElements{1000};
// initialized in main(), because otherwise calling the constructor for
// `Element` will fail due to its internal allocators not being initialized yet
Element* elements;

bool split_points[kNumElements];
int start_index_of_list[kNumElements];

// Mark that we will split at prime-numbered indices.
void PrimeSieve() {
  split_points[2] = true;
  for (int i = 3; i < kNumElements; i += 2)
    split_points[i] = true;
  for (int64_t i = 3; i * i < kNumElements; i += 2) {
    if (split_points[i]) {
      for (int64_t j = i * i; j < kNumElements; j += 2 * i) {
        split_points[j] = false;
      }
    }
  }
}

int main() {
  Element::Initialize();
  pbbs::random r;
  elements = pbbs::new_array_no_init<Element>(kNumElements);
  parallel_for (int i = 0; i < kNumElements; i++) {
    new (&elements[i]) Element(r.ith_rand(i));
  }

  PrimeSieve();

  int start_index{0};
  for (int i = 0; i < kNumElements; i++) {
    start_index_of_list[i] = start_index;
    if (split_points[i]) {
      start_index = i + 1;
    }
  }
  start_index_of_list[0] = start_index_of_list[1] = start_index_of_list[2]
    = start_index % kNumElements;

  parallel_for (int i = 0; i < kNumElements; i++) {
    Element* representative_i{elements[i].FindRepresentative()};
    for (int j = i + 1; j < kNumElements; j++) {
      assert(representative_i != elements[j].FindRepresentative());
    }
  }

  // Join all elements together
  parallel_for (int i = 0; i < kNumElements - 1; i++) {
    Element::Join(&elements[i], &elements[i + 1]);
  }

  Element* representative_0{elements[0].FindRepresentative()};
  parallel_for (int i = 0; i < kNumElements; i++) {
    assert(representative_0 == elements[i].FindRepresentative());
  }

  // Join into one big cycle
  Element::Join(&elements[kNumElements - 1], &elements[0]);

  representative_0 = elements[0].FindRepresentative();
  parallel_for (int i = 0; i < kNumElements; i++) {
    assert(representative_0 == elements[i].FindRepresentative());
  }

  // Split into lists
  parallel_for (int i = 0; i < kNumElements; i++) {
    if (split_points[i]) {
      elements[i].Split();
    }
  }

  parallel_for (int i = 0; i < kNumElements; i++) {
    const int start{start_index_of_list[i]};
    assert(elements[start].FindRepresentative() ==
        elements[i].FindRepresentative());
    if (start > 0) {
      assert(elements[start - 1].FindRepresentative() !=
          elements[i].FindRepresentative());
    }
  }

  // Join individual lists into individual cycles
  parallel_for (int i = 0; i < kNumElements; i++) {
    if (split_points[i]) {
      Element::Join(&elements[i], &elements[start_index_of_list[i]]);
    }
  }

  parallel_for (int i = 0; i < kNumElements; i++) {
    const int start{start_index_of_list[i]};
    assert(elements[start].FindRepresentative() ==
        elements[i].FindRepresentative());
    if (start > 0) {
      assert(elements[start - 1].FindRepresentative() !=
          elements[i].FindRepresentative());
    }
  }

  // Break cycles back into lists
  parallel_for (int i = 0; i < kNumElements; i++) {
    if (split_points[i]) {
      elements[i].Split();
    }
  }

  // Join lists together
  parallel_for (int i = 0; i < kNumElements; i++) {
    if (split_points[i]) {
      Element::Join(&elements[i], &elements[(i + 1) % kNumElements]);
    }
  }

  representative_0 = elements[0].FindRepresentative();
  parallel_for (int i = 0; i < kNumElements; i++) {
    assert(representative_0 == elements[i].FindRepresentative());
  }

  pbbs::delete_array(elements, kNumElements);
  Element::Finish();

  cout << "Test complete." << endl;

  return 0;
}
