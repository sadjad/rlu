#include "rlu.hh"

using namespace std;
using namespace rlu::metadata;

Thread::Thread(const shared_ptr<Global>& global_context)
    : global_context_(global_context) {}
