#include "list.hh"

using namespace std;
using namespace rlu::list;

template <class T>
List<T>::List()
{
  // creating a min-node and a max-node
  constexpr size_t NODE_LEN = sizeof( Node<T> );
  head_ = reinterpret_cast<Node<T>*>( rlu::alloc( NODE_LEN ) );
  Node<T>* tail = reinterpret_cast<Node<T>*>( rlu::alloc( NODE_LEN ) );

  head_->value = numeric_limits<T>::min();
  head_->next = tail;

  tail->value = numeric_limits<T>::max();
  tail->next = nullptr;
}
