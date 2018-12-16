#pragma once

#include <sequence/skip_list/include/skip_list_base.hpp>

namespace skip_list {

class Element : public ElementBase<Element> {
 public:
  Element(size_t random_int) : ElementBase<Element>(random_int) {}

 private:
  friend class ElementBase<Element>;
};

} // namespace skip_list
