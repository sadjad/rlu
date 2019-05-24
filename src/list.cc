#include "list.hh"

using namespace std;
using namespace rlu;

template <class T>
List<T>::List()
{
  // creating a min-node and a max-node
  auto tail = mem::alloc<Node<T>>(numeric_limits<T>::max(), nullptr);
  head_ = mem::alloc<Node<T>>(numeric_limits<T>::min(), tail);
}

// This code is from Listing (2)

template <class T>
bool List<T>::add(rlu::context::Thread& thread_ctx, const T value)
{
restart:
  bool added = false;
  thread_ctx.reader_lock();

  auto prev = thread_ctx.dereference(head_);
  auto next = thread_ctx.dereference(prev->next);

  while (next->value < value) {
    prev = next;
    next = thread_ctx.dereference(prev->next);
  }

  if (next->value != value) {
    if (!thread_ctx.try_lock(prev) || !thread_ctx.try_lock(next)) {
      thread_ctx.abort();
      goto restart;
    }

    auto node = mem::alloc<Node<T>>(value);
    thread_ctx.assign(node->next, next);
    thread_ctx.assign(prev->next, node);
    added = true;
  }

  thread_ctx.reader_unlock();
  return added;
}

template <class T>
bool List<T>::erase(context::Thread& thread_ctx, const T value)
{
restart:
  bool found = false;
  thread_ctx.reader_lock();

  auto prev = thread_ctx.dereference(head_);
  auto next = thread_ctx.dereference(prev->next);

  while (next->value < value) {
    prev = next;
    next = thread_ctx.dereference(prev->next);
  }

  if (next->value == value) {
    if (!thread_ctx.try_lock(prev) || !thread_ctx.try_lock(next)) {
      thread_ctx.abort();
      goto restart;
    }

    auto node = thread_ctx.dereference(next->next);
    thread_ctx.assign(prev->next, node);
    mem::free(next);
    found = true;
  }

  thread_ctx.reader_unlock();
  return found;
}

template <class T>
bool List<T>::contains(context::Thread& thread_ctx, const T value)
{
  thread_ctx.reader_lock();

  for (auto node = head_; node != nullptr; node = node->next) {
    node = thread_ctx.dereference(node);
    if (node->value == value) return true;
  }

  thread_ctx.reader_unlock();
  return false;
}
