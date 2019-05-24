#include "rcu-list.hh"

#include <random>

using namespace std;
using namespace rcu;

template <class T>
List<T>::List()
{
  // creating a min-node and a max-node
  auto tail = new Node<T>(numeric_limits<T>::max(), nullptr);
  head_ = new Node<T>(numeric_limits<T>::min(), tail);
}

/*
 * creates a list with `n` random numbers (not thread-safe)
 */
template <class T>
List<T>::List(const size_t n, const T min, const T max) : List()
{
  random_device dev;
  mt19937 rng{dev()};
  uniform_int_distribution<T> distribution{min, max};

  size_t count = 0;

  while (count != n) {
    const T candidate = distribution(rng);
    auto prev = head_;
    auto next = head_->next;

    while (next->value < candidate) {
      prev = next;
      next = prev->next;
    }

    if (next->value != candidate) {
      count++;
      prev->next = new Node<T>(candidate, next);
    }
  }
}

// This code is from Listing (2)

template <class T>
bool List<T>::add(const T value)
{
  bool added = false;

  unique_lock<mutex> lock{write_mutex_};

  auto prev = head_;
  auto next = prev->next;

  while (next->value < value) {
    prev = next;
    next = prev->next;
  }

  if (next->value != value) {
    auto node = new Node<T>(value, next);
    prev->next = node;
    added = true;
  }

  return added;
}

template <class T>
bool List<T>::erase(const T value)
{
  bool erased = false;

  unique_lock<mutex> lock{write_mutex_};

  auto prev = head_;
  auto next = prev->next;

  while (next->value < value) {
    prev = next;
    next = prev->next;
  }

  if (next->value == value) {
    prev->next = next->next;
    delete[] next;
    erased = true;
  }

  return erased;
}

template <class T>
bool List<T>::contains(const T value)
{
  rcu_read_lock();
  List<T>::NodePtr node = head_->next;  // skip the head

  for (; node != nullptr; node = node->next) {
    if (node->value >= value) break;
  }

  rcu_read_unlock();
  return (node != nullptr && node->value == value);
}
