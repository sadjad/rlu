#include "list.hh"

using namespace std;
using namespace rlu;

template <class T>
List<T>::List()
{
  // creating a min-node and a max-node
  head_ = mem::alloc<Node<T>>();
  auto tail = mem::alloc<Node<T>>();

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

  auto prev = thread_ctx.dereference(head_);
  auto next = thread_ctx.dereference(prev->next);

  while (next->value < value) {
    prev = next;
    next = thread_ctx.dereference(prev->next);
  }

  if (next->value != value) {
    if (!thread_ctx.try_lock(prev) or
        !thread_ctx.try_lock(next)) {
      thread_ctx.abort();
      goto restart;
    }

    auto node = mem::alloc<Node<T>>();
    node->value = value;
    thread_ctx.assign(node->next, next);
    thread_ctx.assign(prev->next, node);
  }

  thread_ctx.reader_unlock();
}
