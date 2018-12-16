#pragma once

#include <sequence/parallel_skip_list/include/skip_list_base.hpp>

namespace parallel_skip_list {

// Basic phase-concurrent skip list. See interface of `ElementBase<T>`.
class Element : public ElementBase<Element> {
 public:
  explicit Element(size_t random_int) : ElementBase<Element>{random_int} {}

 private:
  friend class ElementBase<Element>;
  static void DerivedInitialize() {}
  static void DerivedFinish() {}
};

}  // namespace parallel_skip_list
