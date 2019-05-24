#ifndef RCU_LIST_HH
#define RCU_LIST_HH

#include <urcu.h>
#include <mutex>

namespace rcu {

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
  std::mutex write_mutex_{};

public:
  List();
  List(const size_t n, const T min, const T max);

  size_t len() const;

  bool add(const T value);
  bool erase(const T value);
  bool contains(const T value);

  NodePtr head() { return head_; }
};

template class List<int32_t>;

}  // namespace rcu

#endif /* RCU_LIST_HH */
