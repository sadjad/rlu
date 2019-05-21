#include "list.hh"

using namespace std;
using namespace rlu;
using namespace rlu::list;

template <class T>
List<T>::List()
{
  // creating a min-node and a max-node
  constexpr size_t NODE_LEN = sizeof(Node<T>);
  head_ = mem::alloc(NODE_LEN);
  Node<T>* tail = mem::alloc(NODE_LEN);

  head_->value = numeric_limits<T>::min();
  head_->next = tail;

  tail->value = numeric_limits<T>::max();
  tail->next = nullptr;
}

// This code is from Listing (2)

template <class T>
void List<T>::add(rlu::context::Thread& thread_ctx, T value)
{
restart:
  thread_ctx.reader_lock();

  Node<T>* prev = thread_ctx.dereference(head_);
  Node<T>* next = thread_ctx.dereference(prev->next);

  while (next->value < value) {
    prev = next;
    next = thread_ctx.dereference(prev->next);
  }

  if (next->value != value) {
    if (!thread_ctx.try_lock(prev) or !thread_ctx.try_lock(next)) {
      thread_ctx.abort();
      goto restart;
    }

    Node<T>* node = mem::alloc(sizeof(Node<T>));
    node->value = value;
    thread_ctx.assign(node->next, next);
    thread_ctx.assign(prev->next, node);
  }

  thread_ctx.reader_unlock();
}
