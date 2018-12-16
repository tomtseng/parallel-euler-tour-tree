#include <sequence/parallel_skip_list/include/augmented_skip_list.hpp>

#include <cassert>

#include <utilities/include/debug.hpp>
#include <utilities/include/utils.h>

using std::make_pair;
using std::pair;
using Element = parallel_skip_list::AugmentedElement;
typedef pair<Element*, Element*> ElementPPair;

constexpr int NumElements{1000};
// initialized in main(), because otherwise calling the constructor for
// `Element` will fail due to its internal allocators not being initialized yet
Element* elements;

bool split_points[NumElements];
int start_index_of_list[NumElements];

// Mark that we will split at prime-numbered indices.
void PrimeSieve() {
  split_points[2] = true;
  for (int i = 3; i < NumElements; i += 2)
    split_points[i] = true;
  for (int64_t i = 3; i * i < NumElements; i += 2) {
    if (split_points[i]) {
      for (int64_t j = i * i; j < NumElements; j += 2 * i) {
        split_points[j] = false;
      }
    }
  }
}

int NaiveGetSize(int idx) {
  int size{1};
  Element* curr{elements[idx].GetPreviousElement()};
  while (curr != nullptr && curr != &elements[idx]) {
    size++;
    curr = curr->GetPreviousElement();
  }
  if (curr == nullptr) {  // acyclic list
    curr = elements[idx].GetNextElement();
    while (curr != nullptr) {
      size++;
      curr = curr->GetNextElement();
    }
  }
  return size;
}

void CheckListSize(int idx) {
  const int true_size{NaiveGetSize(idx)};
  const int get_size{elements[idx].GetSum()};
  if (true_size != get_size) TRACE(idx _ true_size _ get_size);
  assert(true_size == get_size);
}

int main() {
  Element::Initialize();
  pbbs::random r;
  elements = pbbs::new_array_no_init<Element>(NumElements);
  parallel_for (int i = 0; i < NumElements; i++) {
    new (&elements[i]) Element(r.ith_rand(i));
  }

  PrimeSieve();

  int start_index{0};
  for (int i = 0; i < NumElements; i++) {
    start_index_of_list[i] = start_index;
    if (split_points[i]) {
      start_index = i + 1;
    }
  }
  start_index_of_list[0] = start_index_of_list[1] = start_index_of_list[2]
    = start_index % NumElements;

  parallel_for (int i = 0; i < NumElements; i++) {
    Element* representative_i{elements[i].FindRepresentative()};
    for (int j = i + 1; j < NumElements; j++) {
      assert(representative_i != elements[j].FindRepresentative());
    }
    CheckListSize(i);
  }

  ElementPPair* joins{pbbs::new_array_no_init<ElementPPair>(NumElements)};
  Element** splits{pbbs::new_array_no_init<Element*>(NumElements)};

  // Join all elements together
  parallel_for (int i = 0; i < NumElements - 1; i++) {
    joins[i] = make_pair(&elements[i], &elements[i + 1]);
  }
  Element::BatchJoin(joins, NumElements - 1);

  Element* representative_0{elements[0].FindRepresentative()};
  parallel_for (int i = 0; i < NumElements; i++) {
    assert(representative_0 == elements[i].FindRepresentative());
    CheckListSize(i);
  }

  // Join into one big cycle
  joins[0] = make_pair(&elements[NumElements - 1], &elements[0]);
  Element::BatchJoin(joins, 1);

  representative_0 = elements[0].FindRepresentative();
  parallel_for (int i = 0; i < NumElements; i++) {
    assert(representative_0 == elements[i].FindRepresentative());
    CheckListSize(i);
  }

  // Split into lists
  int len{0};
  for (int i = 0; i < NumElements; i++) {
    if (split_points[i]) {
      splits[len++] = &elements[i];
    }
  }
  Element::BatchSplit(splits, len);

  parallel_for (int i = 0; i < NumElements; i++) {
    const int start{start_index_of_list[i]};
    assert(elements[start].FindRepresentative() ==
        elements[i].FindRepresentative());
    if (start > 0) {
      assert(elements[start - 1].FindRepresentative() !=
          elements[i].FindRepresentative());
    }
    CheckListSize(i);
  }

  // Join individual lists into individual cycles
  len = 0;
  for (int i = 0; i < NumElements; i++) {
    if (split_points[i]) {
      joins[len++] = make_pair(&elements[i], &elements[start_index_of_list[i]]);
    }
  }
  Element::BatchJoin(joins, len);

  parallel_for (int i = 0; i < NumElements; i++) {
    const int start{start_index_of_list[i]};
    assert(elements[start].FindRepresentative() ==
        elements[i].FindRepresentative());
    if (start > 0) {
      assert(elements[start - 1].FindRepresentative() !=
          elements[i].FindRepresentative());
    }
    CheckListSize(i);
  }

  // Break cycles back into lists
  Element::BatchSplit(splits, len);

  // Join lists together
  len = 0;
  for (int i = 0; i < NumElements; i++) {
    if (split_points[i]) {
      joins[len++] = make_pair(&elements[i], &elements[(i + 1) % NumElements]);
    }
  }
  Element::BatchJoin(joins, len);

  representative_0 = elements[0].FindRepresentative();
  parallel_for (int i = 0; i < NumElements; i++) {
    assert(representative_0 == elements[i].FindRepresentative());
    CheckListSize(i);
  }

  pbbs::delete_array(joins, NumElements);
  pbbs::delete_array(splits, NumElements);
  pbbs::delete_array(elements, NumElements);
  Element::Finish();

  cout << "Test complete." << endl;

  return 0;
}
