#ifndef LINKED_LIST_HH
#define LINKED_LIST_HH

#include "rlu.hh"

namespace rlu {

template <class T>
struct Node {
  T value;
  Node<T>* next;

  Node(const T v = {}, Node<T>* next = nullptr) : value(v), next(next) {}
};

template <class T>
class List {
public:
  using NodePtr = Node<T>*;

private:
  NodePtr head_{nullptr};

public:
  List();
  List(const size_t n, const T min, const T max);

  size_t len() const;

  bool add(context::Thread& thread_ctx, const T value);
  bool erase(context::Thread& thread_ctx, const T value);
  bool contains(context::Thread& thread_ctx, const T value);

  NodePtr head() { return head_; }
};

template class List<int32_t>;

}  // namespace rlu

#endif /* LINKED_LIST_HH */
