// Concurrent array allocator for small arrays.
// Elements of the array are not initialized. If T is an object, the allocator
// does not call constructors or destructors for T.
//
// Do not statically initialize this. This object depends on list_allocator
// being statically initialized before it can be initialized.
//
// Implementation: to handle an allocation request of length k, round k up to
// the nearest power of 2 and give an array of that size. Thus if the max array
// size is n, there are log(n) sizes to allocate. We handle each of these sizes
// with a concurrent fixed-size allocator.
#pragma once

#include <cassert>

#include <utilities/include/list_allocator.h>

namespace concurrent_array_allocator {

constexpr int kMaxArrayLength{32};

template <typename T>
class Allocator {
 public:
  Allocator();
  // Note that this destructor will call `finish()` on several
  // `list_allocator<T>`s, so be sure that allocators of the same type aren't
  // used elsewhere.
  // TODO(tomtseng): We shouldn't have to think about other objects when calling
  // a destructor.
  ~Allocator();
  Allocator(const Allocator&) = delete;
  Allocator(Allocator&&) = delete;
  Allocator& operator=(const Allocator&) = delete;
  Allocator& operator=(Allocator&&) = delete;

  T* Allocate(int length);
  void Free(T* arr, int length);

 private:
  static list_allocator<T[1]> allocator0;
  static list_allocator<T[2]> allocator1;
  static list_allocator<T[4]> allocator2;
  static list_allocator<T[8]> allocator3;
  static list_allocator<T[16]> allocator4;
  static list_allocator<T[32]> allocator5;
};

///////////////////////////////////////////////////////////////////////////////
//                           Implementation below.                           //
///////////////////////////////////////////////////////////////////////////////

template <typename T>
Allocator<T>::Allocator() {
  allocator0.init();
  allocator1.init();
  allocator2.init();
  allocator3.init();
  allocator4.init();
  allocator5.init();
}

template <typename T>
Allocator<T>::~Allocator() {
  allocator0.finish();
  allocator1.finish();
  allocator2.finish();
  allocator3.finish();
  allocator4.finish();
  allocator5.finish();
}

template <typename T>
T* Allocator<T>::Allocate(int length) {
  switch (pbbs::log2_up(length)) {
    case 0: return *allocator0.alloc();
    case 1: return *allocator1.alloc();
    case 2: return *allocator2.alloc();
    case 3: return *allocator3.alloc();
    case 4: return *allocator4.alloc();
    case 5: return *allocator5.alloc();
    default: return nullptr;
  }
}

template <typename T>
void Allocator<T>::Free(T* arr, int length) {
  // To justify this `reinterpret_cast`, we use that a pointer to a static array
  // and a static array itself both point to the same address: the base of the
  // array.
  switch (pbbs::log2_up(length)) {
    case 0: allocator0.free(reinterpret_cast<T(*)[1]>(arr)); break;
    case 1: allocator1.free(reinterpret_cast<T(*)[2]>(arr)); break;
    case 2: allocator2.free(reinterpret_cast<T(*)[4]>(arr)); break;
    case 3: allocator3.free(reinterpret_cast<T(*)[8]>(arr)); break;
    case 4: allocator4.free(reinterpret_cast<T(*)[16]>(arr)); break;
    case 5: allocator5.free(reinterpret_cast<T(*)[32]>(arr)); break;
    default: assert(false);
  }
}

}  // namespace concurrent_array_allocator
