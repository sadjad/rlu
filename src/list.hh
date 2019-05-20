#ifndef LINKED_LIST_HH
#define LINKED_LIST_HH

#include "rlu.hh"

namespace rlu {
namespace list {

template <class T>
class Node {
  T value;
  Node<T>* next{nullptr};
};

template <class T>
class List {
private:
  Node<T>* head_;

public:
  List();

  size_t len() const;

  void add( context::Thread& thread_ctx, T val );
  void erase( context::Thread& thread_ctx, T val );
};

}  // namespace list
}  // namespace rlu

#endif /* LINKED_LIST_HH */
